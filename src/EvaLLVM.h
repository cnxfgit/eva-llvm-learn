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

class EvaLLVM
{
public:
    EvaLLVM()
    {
        moduleInit();
        setupExternFunctions();
    }

    void exec(const std::string &program)
    {
        compile();
        module->print(llvm::outs(), nullptr);
        std::cout << "\n";
        saveModuleToFile("./out.ll");
    }

    ~EvaLLVM() = default;

private:
    llvm::Function *fn;

    std::unique_ptr<llvm::LLVMContext> ctx;

    std::unique_ptr<llvm::Module> module;

    std::unique_ptr<llvm::IRBuilder<>> builder;

    void compile()
    {
        fn = createFunction("main", llvm::FunctionType::get(builder->getInt32Ty(), false));
        gen();
        builder->CreateRet(builder->getInt32(0));
    }

    llvm::Value *gen()
    {
        auto str = builder->CreateGlobalStringPtr("hello\n");
        auto printfFn = module->getFunction("printf");

        std::vector<llvm::Value*> args{str};
        return builder->CreateCall(printfFn, args);
    }

    void setupExternFunctions()
    {
        auto bytePtrTy = builder->getInt8Ty()->getPointerTo();
        module->getOrInsertFunction("printf",
                                    llvm::FunctionType::get(builder->getInt32Ty(), bytePtrTy, true));
    }

    llvm::Function *createFunction(const std::string &fnName, llvm::FunctionType *fnType)
    {
        auto fn = module->getFunction(fnName);

        if (fn == nullptr)
        {
            fn = createFunctionProto(fnName, fnType);
        }

        createFunctionBlock(fn);

        return fn;
    }

    llvm::Function *createFunctionProto(const std::string &fnName, llvm::FunctionType *fnType)
    {
        auto fn = llvm::Function::Create(fnType, llvm::Function::ExternalLinkage, fnName, *module);

        verifyFunction(*fn);

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
    }
};

#endif