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
#include <assert.h>
#include <node_api.h>
#include <stdio.h>

#define arraysize(a) ((int)(sizeof(a) / sizeof(*a)))

static void checkStatus(napi_env env, napi_status status, const char *file, int line) {
  if (status == napi_ok)
    return;

  napi_value exception;

  assert(napi_get_and_clear_last_exception(env, &exception) == napi_ok);
  assert(napi_fatal_exception(env, exception) == napi_ok);

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
          napi_value element;

          CHECK(napi_create_string_utf8(env, funcTable->string[funcIndex], NAPI_AUTO_LENGTH, &element));
          CHECK(napi_set_element(env, array, funcIndex, element));
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
          napi_value element;

          CHECK(napi_create_string_utf8(env, funcTable->string[funcIndex], NAPI_AUTO_LENGTH, &element));
          CHECK(napi_set_element(env, array, funcCount++, element));
        }

        TA_FuncTableFree(funcTable);
      }
    }

    TA_GroupTableFree(groupTable);
  }

  return array;
}

static napi_value init(napi_env env, napi_value exports) {
  napi_property_descriptor props[] = {
    DECLARE_NAPI_METHOD(getFunctionGroups),
    DECLARE_NAPI_METHOD(getFunctions),
  };
  CHECK(napi_define_properties(env, exports, arraysize(props), props));

  return exports;
}

NAPI_MODULE(NODE_GYP_MODULE_NAME, init)
