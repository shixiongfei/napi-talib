/*
 * binding.cpp
 *
 * Copyright (c) 2024 Xiongfei Shi
 *
 * Author: Xiongfei Shi <xiongfei.shi(a)icloud.com>
 * License: Apache-2.0
 *
 * https://github.com/shixiongfei/napi-talib
 */

#include "ta_abstract.h"
#include <stdio.h>
#include <node_api.h>

#define arraysize(a) ((int)(sizeof(a) / sizeof(*a)))

static void checkStatus(napi_env env, napi_status status, const char *file, int line) {
  if (napi_ok == status)
    return;

  napi_value exception;

  if (napi_ok == napi_get_and_clear_last_exception(env, &exception))
    napi_fatal_exception(env, exception);

  fprintf(stderr, "NAPI check status = %d, file: %s, line: %d\n", status, file, line);
}
#define CHECK(__expression__) checkStatus(env, __expression__, __FILE__, __LINE__)

#define DECLARE_NAPI_METHOD_(name, method)                                     \
  {name, 0, method, 0, 0, 0, napi_default, 0}
#define DECLARE_NAPI_METHOD(method) DECLARE_NAPI_METHOD_(#method, method)

static napi_value getFunctionGroups(napi_env env, napi_callback_info info) {
  napi_value object;
  TA_StringTable *groupTable;

  CHECK(napi_create_object(env, &object));

  if (TA_SUCCESS == TA_GroupTableAlloc(&groupTable)) {
    TA_StringTable *funcTable;

    for (unsigned int groupIndex = 0; groupIndex < groupTable->size; ++groupIndex) {
      if (TA_SUCCESS == TA_FuncTableAlloc(groupTable->string[groupIndex], &funcTable)) {
        napi_value array;

        CHECK(napi_create_array_with_length(env, funcTable->size, &array));

        for (unsigned int funcIndex = 0; funcIndex < funcTable->size; ++funcIndex) {
          napi_value value;

          CHECK(napi_create_string_utf8(env, funcTable->string[funcIndex], NAPI_AUTO_LENGTH, &value));
          CHECK(napi_set_element(env, array, funcIndex, value));
        }

        CHECK(napi_set_named_property(env, object, groupTable->string[groupIndex], array));

        TA_FuncTableFree(funcTable);
      }
    }

    TA_GroupTableFree(groupTable);
  }

  return object;
}

static napi_value getFunctions(napi_env env, napi_callback_info info) {
  napi_value array;
  TA_StringTable *groupTable;
  int funcCount = 0;

  CHECK(napi_create_array(env, &array));

  if (TA_SUCCESS == TA_GroupTableAlloc(&groupTable)) {
    TA_StringTable *funcTable;

    for (unsigned int groupIndex = 0; groupIndex < groupTable->size; ++groupIndex) {
      if (TA_SUCCESS == TA_FuncTableAlloc(groupTable->string[groupIndex], &funcTable)) {
        for (unsigned int funcIndex = 0; funcIndex < funcTable->size; ++funcIndex) {
          napi_value value;

          CHECK(napi_create_string_utf8(env, funcTable->string[funcIndex], NAPI_AUTO_LENGTH, &value));
          CHECK(napi_set_element(env, array, funcCount++, value));
        }

        TA_FuncTableFree(funcTable);
      }
    }

    TA_GroupTableFree(groupTable);
  }

  return array;
}

static napi_value explain(napi_env env, napi_callback_info info) {
  size_t argc = 1;
  napi_value argv, jsthis, object, value, inputs, optInputs, outputs;
  napi_valuetype valuetype;
  char funcName[64];
  const TA_FuncHandle *funcHandle;
  const TA_FuncInfo *funcInfo;
  const TA_InputParameterInfo *inputParaminfo;
  const TA_OptInputParameterInfo *optParaminfo;
  const TA_OutputParameterInfo *outputParaminfo;

  CHECK(napi_get_cb_info(env, info, &argc, &argv, &jsthis, nullptr));
  CHECK(napi_typeof(env, argv, &valuetype));

  if (valuetype != napi_string) {
    napi_throw_type_error(env, nullptr, "The argument must be a String");
    return jsthis;
  }

  CHECK(napi_get_value_string_utf8(env, argv, funcName, sizeof(funcName), nullptr));

  if (TA_SUCCESS != TA_GetFuncHandle(funcName, &funcHandle)) {
    CHECK(napi_get_undefined(env, &object));
    return object;
  }

  if (TA_SUCCESS != TA_GetFuncInfo(funcHandle, &funcInfo)) {
    CHECK(napi_get_undefined(env, &object));
    return object;
  }

  CHECK(napi_create_object(env, &object));

  CHECK(napi_create_string_utf8(env, funcInfo->name, NAPI_AUTO_LENGTH, &value));
  CHECK(napi_set_named_property(env, object, "name", value));

  CHECK(napi_create_string_utf8(env, funcInfo->group, NAPI_AUTO_LENGTH, &value));
  CHECK(napi_set_named_property(env, object, "group", value));

  CHECK(napi_create_string_utf8(env, funcInfo->hint, NAPI_AUTO_LENGTH, &value));
  CHECK(napi_set_named_property(env, object, "hint", value));

  CHECK(napi_create_array(env, &inputs));

  for (unsigned int i = 0; i < funcInfo->nbInput; ++i) {
    napi_value param, flags;
    unsigned int flagsCount = 0;

    TA_GetInputParameterInfo(funcInfo->handle, i, &inputParaminfo);

    CHECK(napi_create_object(env, &param));
    CHECK(napi_create_array(env, &flags));

    CHECK(napi_create_string_utf8(env, inputParaminfo->paramName, NAPI_AUTO_LENGTH, &value));
    CHECK(napi_set_named_property(env, param, "name", value));

    switch (inputParaminfo->type) {
      case TA_Input_Price:
        CHECK(napi_create_string_utf8(env, "price", NAPI_AUTO_LENGTH, &value));
        CHECK(napi_set_named_property(env, param, "type", value));
        break;

      case TA_Input_Real:
        CHECK(napi_create_string_utf8(env, "real", NAPI_AUTO_LENGTH, &value));
        CHECK(napi_set_named_property(env, param, "type", value));
        break;

      case TA_Input_Integer:
        CHECK(napi_create_string_utf8(env, "integer", NAPI_AUTO_LENGTH, &value));
        CHECK(napi_set_named_property(env, param, "type", value));
        break;
    }

    if (inputParaminfo->flags & TA_IN_PRICE_OPEN) {
      CHECK(napi_create_string_utf8(env, "open", NAPI_AUTO_LENGTH, &value));
      CHECK(napi_set_element(env, flags, flagsCount++, value));
    }

    if (inputParaminfo->flags & TA_IN_PRICE_HIGH) {
      CHECK(napi_create_string_utf8(env, "high", NAPI_AUTO_LENGTH, &value));
      CHECK(napi_set_element(env, flags, flagsCount++, value));
    }

    if (inputParaminfo->flags & TA_IN_PRICE_LOW) {
      CHECK(napi_create_string_utf8(env, "low", NAPI_AUTO_LENGTH, &value));
      CHECK(napi_set_element(env, flags, flagsCount++, value));
    }

    if (inputParaminfo->flags & TA_IN_PRICE_CLOSE) {
      CHECK(napi_create_string_utf8(env, "close", NAPI_AUTO_LENGTH, &value));
      CHECK(napi_set_element(env, flags, flagsCount++, value));
    }

    if (inputParaminfo->flags & TA_IN_PRICE_VOLUME) {
      CHECK(napi_create_string_utf8(env, "volume", NAPI_AUTO_LENGTH, &value));
      CHECK(napi_set_element(env, flags, flagsCount++, value));
    }

    if (inputParaminfo->flags & TA_IN_PRICE_OPENINTEREST) {
      CHECK(napi_create_string_utf8(env, "openinterest", NAPI_AUTO_LENGTH, &value));
      CHECK(napi_set_element(env, flags, flagsCount++, value));
    }

    if (inputParaminfo->flags & TA_IN_PRICE_TIMESTAMP) {
      CHECK(napi_create_string_utf8(env, "timestamp", NAPI_AUTO_LENGTH, &value));
      CHECK(napi_set_element(env, flags, flagsCount++, value));
    }

    CHECK(napi_set_named_property(env, param, "flags", flags));

    CHECK(napi_set_element(env, inputs, i, param));
  }

  CHECK(napi_set_named_property(env, object, "inputs", inputs));

  CHECK(napi_create_array(env, &optInputs));

  for (unsigned int i = 0; i < funcInfo->nbOptInput; ++i) {
    napi_value param, flags;
    unsigned int flagsCount = 0;

    TA_GetOptInputParameterInfo(funcInfo->handle, i, &optParaminfo);

    CHECK(napi_create_object(env, &param));
    CHECK(napi_create_array(env, &flags));

    CHECK(napi_create_string_utf8(env, optParaminfo->paramName, NAPI_AUTO_LENGTH, &value));
    CHECK(napi_set_named_property(env, param, "name", value));

    CHECK(napi_create_string_utf8(env, optParaminfo->displayName, NAPI_AUTO_LENGTH, &value));
    CHECK(napi_set_named_property(env, param, "displayName", value));

    CHECK(napi_create_double(env, optParaminfo->defaultValue, &value));
    CHECK(napi_set_named_property(env, param, "defaultValue", value));

    CHECK(napi_create_string_utf8(env, optParaminfo->hint, NAPI_AUTO_LENGTH, &value));
    CHECK(napi_set_named_property(env, param, "hint", value));

    switch (optParaminfo->type) {
      case TA_OptInput_RealRange:
        CHECK(napi_create_string_utf8(env, "real_range", NAPI_AUTO_LENGTH, &value));
        CHECK(napi_set_named_property(env, param, "type", value));
        break;

      case TA_OptInput_RealList:
        CHECK(napi_create_string_utf8(env, "real_list", NAPI_AUTO_LENGTH, &value));
        CHECK(napi_set_named_property(env, param, "type", value));
        break;

      case TA_OptInput_IntegerRange:
        CHECK(napi_create_string_utf8(env, "integer_range", NAPI_AUTO_LENGTH, &value));
        CHECK(napi_set_named_property(env, param, "type", value));
        break;

      case TA_OptInput_IntegerList:
        CHECK(napi_create_string_utf8(env, "integer_list", NAPI_AUTO_LENGTH, &value));
        CHECK(napi_set_named_property(env, param, "type", value));
        break;
    }

    if (optParaminfo->flags & TA_OPTIN_IS_PERCENT) {
      CHECK(napi_create_string_utf8(env, "percent", NAPI_AUTO_LENGTH, &value));
      CHECK(napi_set_element(env, flags, flagsCount++, value));
    }

    if (optParaminfo->flags & TA_OPTIN_IS_DEGREE) {
      CHECK(napi_create_string_utf8(env, "degree", NAPI_AUTO_LENGTH, &value));
      CHECK(napi_set_element(env, flags, flagsCount++, value));
    }

    if (optParaminfo->flags & TA_OPTIN_IS_CURRENCY) {
      CHECK(napi_create_string_utf8(env, "currency", NAPI_AUTO_LENGTH, &value));
      CHECK(napi_set_element(env, flags, flagsCount++, value));
    }

    if (optParaminfo->flags & TA_OPTIN_ADVANCED) {
      CHECK(napi_create_string_utf8(env, "advanced", NAPI_AUTO_LENGTH, &value));
      CHECK(napi_set_element(env, flags, flagsCount++, value));
    }

    CHECK(napi_set_named_property(env, param, "flags", flags));

    CHECK(napi_set_element(env, optInputs, i, param));
  }

  CHECK(napi_set_named_property(env, object, "optInputs", optInputs));

  CHECK(napi_create_array(env, &outputs));

  for (unsigned int i = 0; i < funcInfo->nbOutput; ++i) {
    napi_value param, flags;
    unsigned int flagsCount = 0;

    TA_GetOutputParameterInfo(funcInfo->handle, i, &outputParaminfo);

    CHECK(napi_create_object(env, &param));
    CHECK(napi_create_array(env, &flags));

    CHECK(napi_create_string_utf8(env, outputParaminfo->paramName, NAPI_AUTO_LENGTH, &value));
    CHECK(napi_set_named_property(env, param, "name", value));

    switch (outputParaminfo->type) {
      case TA_Output_Real:
        CHECK(napi_create_string_utf8(env, "real", NAPI_AUTO_LENGTH, &value));
        CHECK(napi_set_named_property(env, param, "type", value));
        break;
      case TA_Output_Integer:
        CHECK(napi_create_string_utf8(env, "integer", NAPI_AUTO_LENGTH, &value));
        CHECK(napi_set_named_property(env, param, "type", value));
        break;
    }

    if (outputParaminfo->flags & TA_OUT_LINE) {
      CHECK(napi_create_string_utf8(env, "line", NAPI_AUTO_LENGTH, &value));
      CHECK(napi_set_element(env, flags, flagsCount++, value));
    }

    if (outputParaminfo->flags & TA_OUT_DOT_LINE) {
      CHECK(napi_create_string_utf8(env, "line_dot", NAPI_AUTO_LENGTH, &value));
      CHECK(napi_set_element(env, flags, flagsCount++, value));
    }

    if (outputParaminfo->flags & TA_OUT_DASH_LINE) {
      CHECK(napi_create_string_utf8(env, "line_dash", NAPI_AUTO_LENGTH, &value));
      CHECK(napi_set_element(env, flags, flagsCount++, value));
    }

    if (outputParaminfo->flags & TA_OUT_DOT) {
      CHECK(napi_create_string_utf8(env, "dot", NAPI_AUTO_LENGTH, &value));
      CHECK(napi_set_element(env, flags, flagsCount++, value));
    }

    if (outputParaminfo->flags & TA_OUT_HISTO) {
      CHECK(napi_create_string_utf8(env, "histogram", NAPI_AUTO_LENGTH, &value));
      CHECK(napi_set_element(env, flags, flagsCount++, value));
    }

    if (outputParaminfo->flags & TA_OUT_PATTERN_BOOL) {
      CHECK(napi_create_string_utf8(env, "pattern_bool", NAPI_AUTO_LENGTH, &value));
      CHECK(napi_set_element(env, flags, flagsCount++, value));
    }

    if (outputParaminfo->flags & TA_OUT_PATTERN_BULL_BEAR) {
      CHECK(napi_create_string_utf8(env, "pattern_bull_bear", NAPI_AUTO_LENGTH, &value));
      CHECK(napi_set_element(env, flags, flagsCount++, value));
    }

    if (outputParaminfo->flags & TA_OUT_PATTERN_STRENGTH) {
      CHECK(napi_create_string_utf8(env, "pattern_strength", NAPI_AUTO_LENGTH, &value));
      CHECK(napi_set_element(env, flags, flagsCount++, value));
    }

    if (outputParaminfo->flags & TA_OUT_POSITIVE) {
      CHECK(napi_create_string_utf8(env, "positive", NAPI_AUTO_LENGTH, &value));
      CHECK(napi_set_element(env, flags, flagsCount++, value));
    }

    if (outputParaminfo->flags & TA_OUT_NEGATIVE) {
      CHECK(napi_create_string_utf8(env, "negative", NAPI_AUTO_LENGTH, &value));
      CHECK(napi_set_element(env, flags, flagsCount++, value));
    }

    if (outputParaminfo->flags & TA_OUT_ZERO) {
      CHECK(napi_create_string_utf8(env, "zero", NAPI_AUTO_LENGTH, &value));
      CHECK(napi_set_element(env, flags, flagsCount++, value));
    }

    if (outputParaminfo->flags & TA_OUT_UPPER_LIMIT) {
      CHECK(napi_create_string_utf8(env, "limit_upper", NAPI_AUTO_LENGTH, &value));
      CHECK(napi_set_element(env, flags, flagsCount++, value));
    }

    if (outputParaminfo->flags & TA_OUT_LOWER_LIMIT) {
      CHECK(napi_create_string_utf8(env, "limit_lower", NAPI_AUTO_LENGTH, &value));
      CHECK(napi_set_element(env, flags, flagsCount++, value));
    }

    CHECK(napi_set_named_property(env, param, "flags", flags));

    CHECK(napi_set_element(env, outputs, i, param));
  }

  CHECK(napi_set_named_property(env, object, "outputs", outputs));

  return object;
}

static napi_value executeSync(napi_env env, napi_value object) {
  return object;
}

static napi_value executeAsync(napi_env env, napi_value object, napi_value callback) {
  napi_value undefined, errmsg, argv[2];

  CHECK(napi_get_undefined(env, &undefined));

  CHECK(napi_create_string_utf8(env, "Not Yet Implemented", NAPI_AUTO_LENGTH, &errmsg));
  CHECK(napi_create_error(env, nullptr, errmsg, &argv[0]));
  argv[1] = undefined;

  CHECK(napi_call_function(env, undefined, callback, 2, argv, nullptr));

  return undefined;
}

static napi_value execute(napi_env env, napi_callback_info info) {
  size_t argc = 2;
  napi_value argv[2], jsthis;
  napi_valuetype valuetype;

  CHECK(napi_get_cb_info(env, info, &argc, argv, &jsthis, nullptr));

  CHECK(napi_typeof(env, argv[0], &valuetype));

  if (valuetype != napi_object) {
    napi_throw_type_error(env, nullptr, "The first argument must be a Object");
    return jsthis;
  }

  CHECK(napi_typeof(env, argv[1], &valuetype));

  return valuetype == napi_function
    ? executeAsync(env, argv[0], argv[1])
    : executeSync(env, argv[0]);
}

static napi_value init(napi_env env, napi_value exports) {
  TA_Initialize();

  napi_property_descriptor props[] = {
    DECLARE_NAPI_METHOD(getFunctionGroups),
    DECLARE_NAPI_METHOD(getFunctions),
    DECLARE_NAPI_METHOD(explain),
    DECLARE_NAPI_METHOD(execute),
  };
  CHECK(napi_define_properties(env, exports, arraysize(props), props));

  return exports;
}

NAPI_MODULE(NODE_GYP_MODULE_NAME, init)
