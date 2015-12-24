#include <nan.h>
#include "../common.h"

#include <sol-platform.h>

using namespace v8;

NAN_METHOD(bind_sol_platform_get_machine_id) {
  VALIDATE_ARGUMENT_COUNT(info, 0);

  const char *machineId = 0;

  machineId = sol_platform_get_machine_id();

  info.GetReturnValue().Set( Nan::New( machineId ).ToLocalChecked() );
}
