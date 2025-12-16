#pragma once
#include <stdexcept>

namespace engine
{
  
/// @brief A base class for all engine exceptions.
class EngineError : public std::exception
{
protected:
    std::string mMsg;
public:
    inline explicit EngineError(std::string_view msg) : mMsg(msg) {}
    explicit EngineError() = default;

    virtual ~EngineError() = default;

    char const *what() const noexcept override { return mMsg.c_str(); }
};

class ResourceError : public EngineError { public: using EngineError::EngineError; };
class InvalidResourceLoaderError : public ResourceError { public: using EngineError::EngineError; };
class InvalidResourceHandleError : public ResourceError { public: using EngineError::EngineError; };

} // namespace engine
