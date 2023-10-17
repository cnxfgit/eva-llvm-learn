#ifndef EvaLLVM_h
#define EvaLLVM_h

#include <string>
#include <memory>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Verifier.h>
#include <iostream>
#include <errno.h>

#include "./Environment.h"
#include "./parser/EvaParser.h"

using syntax::EvaParser;
using Env = std::shared_ptr<Environment>;

struct ClassInfo
{
    llvm::StructType *cls;
    llvm::StructType *parent;
    std::map<std::string, llvm::Type *> fieldsMap;
    std::map<std::string, llvm::Function *> methodsMap;
};

#define GEN_BINARY_OP(Op, varName)             \
    do                                         \
    {                                          \
        auto op1 = gen(exp.list[1], env);      \
        auto op2 = gen(exp.list[2], env);      \
        return builder->Op(op1, op2, varName); \
    } while (false);

class EvaLLVM
{
public:
    EvaLLVM() : parser(std::make_unique<EvaParser>())
    {
        moduleInit();
        setupExternFunctions();
        setupGlobalEnvironment();
    }

    void exec(const std::string &program)
    {
        auto ast = parser->parse("(begin " + program + ")");
        compile(ast);
        module->print(llvm::outs(), nullptr);
        std::cout << "\n";
        saveModuleToFile("./out.ll");
    }

    ~EvaLLVM() = default;

private:
    std::unique_ptr<EvaParser> parser;

    std::shared_ptr<Environment> GlobalEnv;

    llvm::StructType *cls = nullptr;

    std::map<std::string, ClassInfo> classMap_;

    llvm::Function *fn;

    std::unique_ptr<llvm::LLVMContext> ctx;

    std::unique_ptr<llvm::Module> module;

    std::unique_ptr<llvm::IRBuilder<>> varsBuilder;

    std::unique_ptr<llvm::IRBuilder<>> builder;

    void compile(const Exp &ast)
    {
        fn = createFunction("main", llvm::FunctionType::get(builder->getInt32Ty(), false), GlobalEnv);
        createGlobalVar("version", builder->getInt32(42));
        gen(ast, GlobalEnv);
        builder->CreateRet(builder->getInt32(0));
    }

    llvm::Value *gen(const Exp &exp, Env env)
    {
        switch (exp.type)
        {
        case ExpType::NUMBER:
            return builder->getInt32(exp.number);
        case ExpType::STRING:
        {
            auto re = std::regex("\\\\n");
            auto str = std::regex_replace(exp.string, re, "\n");
            return builder->CreateGlobalStringPtr(str);
        }
        case ExpType::SYMBOL:
            if (exp.string == "true" || exp.string == "false")
            {
                return builder->getInt1(exp.string == "true" ? true : false);
            }
            else
            {
                auto varName = exp.string;
                auto value = env->lookup(varName);

                if (auto localVar = llvm::dyn_cast<llvm::AllocaInst>(value))
                {
                    return builder->CreateLoad(localVar->getAllocatedType(),
                                               localVar, varName.c_str());
                }

                else if (auto globalVar = llvm::dyn_cast<llvm::GlobalVariable>(value))
                {
                    return builder->CreateLoad(globalVar->getInitializer()->getType(),
                                               globalVar, varName.c_str());
                }

                else
                {
                    return value;
                }
            }

            return builder->getInt32(0);
        case ExpType::LIST:
            auto tag = exp.list[0];

            if (tag.type == ExpType::SYMBOL)
            {
                auto op = tag.string;

                if (op == "+")
                {
                    GEN_BINARY_OP(CreateAdd, "tmpadd");
                }
                else if (op == "-")
                {
                    GEN_BINARY_OP(CreateSub, "tmpsub");
                }
                else if (op == "*")
                {
                    GEN_BINARY_OP(CreateMul, "tmpmul");
                }
                else if (op == "/")
                {
                    GEN_BINARY_OP(CreateSDiv, "tmpdiv");
                }
                else if (op == ">")
                {
                    GEN_BINARY_OP(CreateICmpUGT, "tmpcmp");
                }
                else if (op == "<")
                {
                    GEN_BINARY_OP(CreateICmpULT, "tmpcmp");
                }
                else if (op == "==")
                {
                    GEN_BINARY_OP(CreateICmpEQ, "tmpcmp");
                }
                else if (op == "!=")
                {
                    GEN_BINARY_OP(CreateICmpNE, "tmpcmp");
                }
                else if (op == ">=")
                {
                    GEN_BINARY_OP(CreateICmpUGE, "tmpcmp");
                }
                else if (op == "<=")
                {
                    GEN_BINARY_OP(CreateICmpULE, "tmpcmp");
                }

                else if (op == "if")
                {
                    auto cond = gen(exp.list[1], env);

                    auto thenBlock = createBB("then", fn);

                    auto elseBlock = createBB("else");
                    auto ifEndBlock = createBB("ifend");

                    builder->CreateCondBr(cond, thenBlock, elseBlock);

                    builder->SetInsertPoint(thenBlock);
                    auto thenRes = gen(exp.list[2], env);
                    builder->CreateBr(ifEndBlock);

                    thenBlock = builder->GetInsertBlock();

                    fn->getBasicBlockList().push_back(elseBlock);
                    builder->SetInsertPoint(elseBlock);
                    auto elseRes = gen(exp.list[3], env);
                    builder->CreateBr(ifEndBlock);
                    elseBlock = builder->GetInsertBlock();

                    fn->getBasicBlockList().push_back(ifEndBlock);
                    builder->SetInsertPoint(ifEndBlock);

                    auto phi = builder->CreatePHI(thenRes->getType(), 2, "tmpif");
                    phi->addIncoming(thenRes, thenBlock);
                    phi->addIncoming(elseRes, elseBlock);

                    return phi;
                }

                else if (op == "while")
                {
                    auto condBlock = createBB("cond", fn);
                    builder->CreateBr(condBlock);

                    auto bodyBlock = createBB("body");
                    auto loopEndBlock = createBB("loopend");

                    builder->SetInsertPoint(condBlock);
                    auto cond = gen(exp.list[1], env);

                    builder->CreateCondBr(cond, bodyBlock, loopEndBlock);

                    fn->getBasicBlockList().push_back(bodyBlock);
                    builder->SetInsertPoint(bodyBlock);
                    gen(exp.list[2], env);
                    builder->CreateBr(condBlock);

                    fn->getBasicBlockList().push_back(loopEndBlock);
                    builder->SetInsertPoint(loopEndBlock);

                    return builder->getInt32(0);
                }

                else if (op == "def")
                {
                    return compileFunction(exp, exp.list[1].string, env);
                }

                if (op == "var")
                {
                    if (cls != nullptr)
                    {
                        return builder->getInt32(0);
                    }

                    auto varNameDecl = exp.list[1];
                    auto varName = extractVarName(varNameDecl);
                    auto init = gen(exp.list[2], env);

                    auto varTy = extractVarType(varNameDecl);
                    auto varBinding = allocVar(varName, varTy, env);
                    return builder->CreateStore(init, varBinding);
                }
                else if (op == "set")
                {
                    auto value = gen(exp.list[2], env);

                    auto varName = exp.list[1].string;

                    auto varBinding = env->lookup(varName);

                    builder->CreateStore(value, varBinding);
                    return value;
                }
                else if (op == "begin")
                {
                    auto blockEnv = std::make_shared<Environment>(
                        std::map<std::string, llvm::Value *>{}, env);

                    llvm::Value *blockRes;
                    for (auto i = 1; i < exp.list.size(); i++)
                    {
                        blockRes = gen(exp.list[i], blockEnv);
                    }
                    return blockRes;
                }
                else if (op == "printf")
                {
                    auto printfFn = module->getFunction("printf");
                    std::vector<llvm::Value *> args{};

                    for (auto i = 1; i < exp.list.size(); i++)
                    {
                        args.push_back(gen(exp.list[i], env));
                    }

                    return builder->CreateCall(printfFn, args);
                }

                else if (op == "class")
                {
                    auto name = exp.list[1].string;

                    auto parent = exp.list[2].string == "null" ? nullptr
                                                               : getClassByName(exp.list[2].string);

                    cls = llvm::StructType::create(*ctx, name);

                    if (parent != nullptr)
                    {
                        inheritClass(cls, parent);
                    }
                    else
                    {
                        classMap_[name] = {cls, parent, {}, {}};
                    }

                    buildClassInfo(cls, exp, env);

                    gen(exp.list[3], env);

                    cls = nullptr;

                    return builder->getInt32(0);
                }

                else
                {
                    auto callable = gen(exp.list[0], env);
                    std::vector<llvm::Value *> args{};
                    for (auto i = 1; i < exp.list.size(); i++)
                    {
                        args.push_back(gen(exp.list[i], env));
                    }

                    auto fn = (llvm::Function *)callable;
                    return builder->CreateCall(fn, args);
                }
            }
        }

        return builder->getInt32(0);
    }

    void buildClassInfo(llvm::StructType *cls, const Exp &clsExp, Env env)
    {
        auto className = clsExp.list[1].string;
        auto classInfo = &classMap_[className];

        auto body = clsExp.list[3];

        for (auto i = 1; i < body.list.size(); i++)
        {
            auto exp = body.list[i];

            if (isVar(exp))
            {
                auto varNameDecl = exp.list[1];

                auto fieldName = extractVarName(varNameDecl);
                auto fieldTy = extractVarType(varNameDecl);

                classInfo->fieldsMap[fieldName] = fieldTy;
            }
            else if (isDef(exp))
            {
                auto methodName = exp.list[1].string;
                auto fnName = className + "_" + methodName;

                classInfo->methodsMap[methodName] =
                    createFunctionProto(fnName, extractFcuntionType(exp), env);
            }
        }

        buildClassBody(cls);
    }

    void buildClassBody(llvm::StructType *cls)
    {
        std::string className{cls->getName().data()};

        auto classInfo = &classMap_[className];
        auto clsFields = std::vector<llvm::Type *>{};
        for (const auto &fieldInfo : classInfo->fieldsMap)
        {
            clsFields.push_back(fieldInfo.second);
        }

        cls->setBody(clsFields, false);
    }

    bool isTaggedList(const Exp &exp, const std::string &tag)
    {
        return exp.type == ExpType::LIST && exp.list[0].type == ExpType::SYMBOL &&
               exp.list[0].string == tag;
    }

    bool isVar(const Exp &exp)
    {
        return isTaggedList(exp, "var");
    }

    bool isDef(const Exp &exp)
    {
        return isTaggedList(exp, "def");
    }

    void inheritClass(llvm::StructType *cls, llvm::StructType *parent)
    {
    }

    llvm::StructType *getClassByName(const std::string &name)
    {
        return llvm::StructType::getTypeByName(*ctx, name);
    }

    std::string extractVarName(const Exp &exp)
    {
        return exp.type == ExpType::LIST ? exp.list[0].string : exp.string;
    }

    llvm::Type *extractVarType(const Exp &exp)
    {
        return exp.type == ExpType::LIST ? getTypeFromString(exp.list[1].string)
                                         : builder->getInt32Ty();
    }

    llvm::Type *getTypeFromString(const std::string &type_)
    {
        if (type_ == "number")
        {
            return builder->getInt32Ty();
        }

        if (type_ == "string")
        {
            return builder->getInt8Ty()->getPointerTo();
        }

        return classMap_[type_].cls->getPointerTo();
    }

    bool hasReturnType(const Exp &fnExp)
    {
        return fnExp.list[3].type == ExpType::SYMBOL &&
               fnExp.list[3].string == "->";
    }

    llvm::FunctionType *extractFcuntionType(const Exp &fnExp)
    {
        auto params = fnExp.list[2];

        auto returnType = hasReturnType(fnExp) ? getTypeFromString(fnExp.list[4].string)
                                               : builder->getInt32Ty();
        std::vector<llvm::Type *> paramTypes{};
        for (auto &param : params.list)
        {
            auto paramName = extractVarName(param);
            auto paramTy = extractVarType(param);

            paramTypes.push_back(
                paramName == "self" ? (llvm::Type *)cls->getPointerTo() : paramTy);
        }

        return llvm::FunctionType::get(returnType, paramTypes, false);
    }

    llvm::Value *compileFunction(const Exp &fnExp, std::string fnName, Env env)
    {
        auto params = fnExp.list[2];
        auto body = hasReturnType(fnExp) ? fnExp.list[5] : fnExp.list[3];

        auto prevFn = fn;
        auto prevBlock = builder->GetInsertBlock();

        auto origName = fnName;
        if (cls != nullptr)
        {
            fnName = std::string(cls->getName().data()) + "_" + fnName;
        }

        auto newFn = createFunction(fnName, extractFcuntionType(fnExp), env);
        fn = newFn;

        auto idx = 0;

        auto fnEnv = std::make_shared<Environment>(
            std::map<std::string, llvm::Value *>{}, env);

        for (auto &arg : fn->args())
        {
            auto param = params.list[idx++];
            auto argName = extractVarName(param);

            arg.setName(argName);

            auto argBinding = allocVar(argName, arg.getType(), fnEnv);
            builder->CreateStore(&arg, argBinding);
        }

        builder->CreateRet(gen(body, fnEnv));
        builder->SetInsertPoint(prevBlock);
        fn = prevFn;
        return newFn;
    }

    llvm::Value *allocVar(const std::string &name, llvm::Type *type_, Env env)
    {
        varsBuilder->SetInsertPoint(&fn->getEntryBlock());
        auto varAlloc = varsBuilder->CreateAlloca(type_, 0, name.c_str());
        env->define(name, varAlloc);

        return varAlloc;
    }

    llvm::GlobalVariable *createGlobalVar(const std::string &name, llvm::Constant *init)
    {
        module->getOrInsertGlobal(name, init->getType());
        auto variable = module->getNamedGlobal(name);
        variable->setAlignment(llvm::MaybeAlign(4));
        variable->setConstant(false);
        variable->setInitializer(init);
        return variable;
    }

    void setupExternFunctions()
    {
        auto bytePtrTy = builder->getInt8Ty()->getPointerTo();
        module->getOrInsertFunction("printf",
                                    llvm::FunctionType::get(builder->getInt32Ty(), bytePtrTy, true));
    }

    llvm::Function *createFunction(const std::string &fnName, llvm::FunctionType *fnType, Env env)
    {
        auto fn = module->getFunction(fnName);

        if (fn == nullptr)
        {
            fn = createFunctionProto(fnName, fnType, env);
        }

        createFunctionBlock(fn);

        return fn;
    }

    llvm::Function *createFunctionProto(const std::string &fnName, llvm::FunctionType *fnType, Env env)
    {
        auto fn = llvm::Function::Create(fnType, llvm::Function::ExternalLinkage, fnName, *module);

        verifyFunction(*fn);

        env->define(fnName, fn);

        return fn;
    }

    void createFunctionBlock(llvm::Function *fn)
    {
        auto entry = createBB("entry", fn);
        builder->SetInsertPoint(entry);
    }

    llvm::BasicBlock *createBB(std::string name, llvm::Function *fn = nullptr)
    {
        return llvm::BasicBlock::Create(*ctx, name, fn);
    }

    void saveModuleToFile(const std::string &fileName)
    {
        std::error_code errorCode;
        llvm::raw_fd_ostream outLL(fileName, errorCode);
        module->print(outLL, nullptr);
    }

    void moduleInit()
    {
        ctx = std::make_unique<llvm::LLVMContext>();
        module = std::make_unique<llvm::Module>("EvaLLVM", *ctx);
        builder = std::make_unique<llvm::IRBuilder<>>(*ctx);
        varsBuilder = std::make_unique<llvm::IRBuilder<>>(*ctx);
    }

    void setupGlobalEnvironment()
    {
        std::map<std::string, llvm::Value *> globalObject{
            {"VERSION", builder->getInt32(42)}};

        std::map<std::string, llvm::Value *> globalRec{};

        for (auto &entry : globalObject)
        {
            globalRec[entry.first] = createGlobalVar(entry.first, (llvm::Constant *)entry.second);
        }

        GlobalEnv = std::make_shared<Environment>(globalRec, nullptr);
    }
};

#endif