#pragma once

#include <stddef.h>
#include <type_traits>
#include <utility>
#include <functional>
#include <iostream>
#include <cstdio>
#include "TaskDependency.h"
// define a callable (std::function, function ptr, member funtion ptr)
namespace Task
{
    template<class ReturnType, size_t allocateSize, size_t align>
    class Callable;
    
    template<class ReturnType, typename... Args, size_t allocateSize, size_t align>
    class Callable <ReturnType(Args ...), allocateSize, align>
    {
        public:
            Callable() = default;
            ~Callable()
            {
                getObject()->~InvokeObject();
            }
            inline ReturnType call(Args... args)
            {
                return getObject()->run(std::forward<Args>(args)...);
            }
            bool isActive()
            {
                return getObject()->isActive();
            }
            template<class T>
            Callable(T&& other)
            {
                using functionType = std::remove_cv_t<std::remove_reference_t<T>>;
                //auto tag = std::is_same_v<functionType, std::function<ReturnType(Args...)>>;
                constexpr bool tag = std::is_invocable_r_v<ReturnType, functionType, Args...>;
                if(!tag)
                {
                    std::cout << "no qualified function object" << std::endl;
                }
                new(funcObj) captureFunctionObject<T>(std::forward<T>(other));
            }

            Callable(ReturnType(*other)(Args...))
            {
                new(funcObj) captureFunctionPointerObject(other);
            }

            template<class T>
            Callable(T* other_1, ReturnType(T::*other)(Args...))
            {
                new(funcObj) captureMemberFunctionPointerObject(other_1, other);
            }
        private:
            struct InvokeObject
            {
                InvokeObject() = default;
                virtual ~InvokeObject() = default;
                virtual ReturnType run(Args... args) = 0;
                virtual bool isActive() const = 0;
            };

            template<class T>
            struct captureFunctionObject: public InvokeObject
            {
                explicit captureFunctionObject(T&& other):obj(std::forward<T>(other))       //explicit forbid the lambda,functionptr -> InvokeObject
                {
                    
                }
                inline ReturnType run(Args... args) override
                {
                    return obj(std::forward<Args>(args)...);
                }
                inline bool isActive() const override
                {
                    return true;
                }
                T obj;
            };

            struct captureFunctionPointerObject: public InvokeObject
            {
                explicit captureFunctionPointerObject(ReturnType(*other)(Args...))
                {
                    obj = other;
                }
                inline ReturnType run(Args... args) override
                {
                    return obj(std::forward<Args>(args)...);
                }
                inline bool isActive() const override
                {
                    return obj != nullptr;
                }
                ReturnType(*obj)(Args...);
            };

            template<class T>
            struct captureMemberFunctionPointerObject: public InvokeObject
            {
                explicit captureMemberFunctionPointerObject(T* other_1, ReturnType(T::*other)(Args...))
                {
                    classObj = other_1;
                    obj = other;
                }
                inline ReturnType run(Args... args) override
                {
                    return (classObj->*obj)(std::forward<Args>(args)...);
                }
                inline bool isActive() const override
                {
                    return obj != nullptr && classObj != nullptr;
                }
                ReturnType(T::*obj)(Args...);
                T* classObj;
            };
            alignas(align) char funcObj[allocateSize];
            InvokeObject* getObject()
            {
                return reinterpret_cast<InvokeObject*>(&funcObj[0]);
            }
    };

    
    class Task
    {
    public:
            Callable<void(), 64, 8> callable;
            std::unique_ptr<TaskDependency> taskDependency;
            template<class T>
            explicit Task(std::unique_ptr<TaskDependency> dep, T&& func)
            {
                taskDependency = std::move(dep);
                callable = std::forward<T>(func);
            }
    };
}