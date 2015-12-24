#ifndef __SOLETTA_NODE_COMMON_H__
#define __SOLETTA_NODE_COMMON_H__

#define SET_FUNCTION(destination, functionName)                              \
  Nan::ForceSet((destination), Nan::New(#functionName).ToLocalChecked(),     \
                Nan::GetFunction(Nan::New<FunctionTemplate>(                 \
                                     bind_##functionName)).ToLocalChecked(), \
                (v8::PropertyAttribute)(v8::DontDelete));

#define SET_CONSTANT_NUMBER(destination, name)                   \
  Nan::ForceSet((destination), Nan::New(#name).ToLocalChecked(), \
                Nan::New((name)),                                \
                (v8::PropertyAttribute)(v8::ReadOnly | v8::DontDelete));

#define VALIDATE_CALLBACK_RETURN_VALUE_TYPE(value, typecheck, message)        \
  if (!value->typecheck()) {                                                  \
    Nan::ThrowTypeError(                                                      \
        message " callback return value type must satisfy " #typecheck "()"); \
  }

#define VALIDATE_ARGUMENT_COUNT(args, length)                               \
  if ((args).Length() < (length)) {                                         \
    return Nan::ThrowRangeError("Argument count must be exactly " #length); \
  }

#define VALIDATE_ARGUMENT_TYPE(args, index, typecheck)                        \
  if (!(args)[(index)]->typecheck()) {                                        \
    return Nan::ThrowTypeError("Argument " #index " must satisfy " #typecheck \
                               "()");                                         \
  }

#define VALIDATE_VALUE_TYPE(value, typecheck, message, failReturn) \
  if (!(value)->typecheck()) {                                     \
    Nan::ThrowTypeError(message " must satisfy " #typecheck "()"); \
    return failReturn;                                             \
  }

#define VALIDATE_VALUE_TYPE_OR_FREE(value, typecheck, message, failReturn, \
                                    pointer_to_free, free_function)        \
  if (!(value)->typecheck()) {                                             \
    Nan::ThrowTypeError(message " must satisfy " #typecheck "()");         \
    free_function((pointer_to_free));                                      \
    return failReturn;                                                     \
  }

#define VALIDATE_ARGUMENT_TYPE_OR_NULL(args, index, typecheck)                \
  if (!((args)[(index)]->typecheck() || (args)[(index)]->IsNull())) {         \
    return Nan::ThrowTypeError("Argument " #index " must satisfy " #typecheck \
                               "() or IsNull()");                             \
  }

#define SET_STRING_IF_NOT_NULL(destination, source, memberName)     \
  if ((source)->memberName) {                                       \
    Nan::Set((destination), Nan::New(#memberName).ToLocalChecked(), \
             Nan::New((source)->memberName).ToLocalChecked());      \
  }

#define SET_VALUE_ON_OBJECT(destination, type, source, memberName) \
  Nan::Set((destination), Nan::New(#memberName).ToLocalChecked(),  \
           Nan::New<type>((source)->memberName));

#define VALIDATE_AND_ASSIGN(destination, memberName, destinationType,         \
                            typecheck, message, failReturn, source, accessor) \
  Local<Value> memberName =                                                   \
      Nan::Get(source, Nan::New(#memberName).ToLocalChecked())                \
          .ToLocalChecked();                                                  \
  VALIDATE_VALUE_TYPE(memberName, typecheck, message "." #memberName,         \
                      failReturn);                                            \
  destination.memberName = (destinationType)memberName->accessor();

#endif /* ndef __SOLETTA_NODE_COMMON_H__ */
