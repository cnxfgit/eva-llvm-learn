#ifndef EvaLLVM_h
#define EvaLLVM_h

#include <string>
#include <memory>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Module.h>
#include <errno.h>

class EvaLLVM
{
public:
    EvaLLVM()
    {
        moduleInit();
    }

    void exec(const std::string &program)
    {

        module->print(llvm::outs(), nullptr);
        saveModuleToFile("./out.ll");
    }

    ~EvaLLVM() = default;

private:
    std::unique_ptr<llvm::LLVMContext> ctx;

    std::unique_ptr<llvm::Module> module;

    std::unique_ptr<llvm::IRBuilder<>> builder;

    void saveModuleToFile(const std::string& fileName){
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