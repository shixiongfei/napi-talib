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
#include <vector>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

typedef struct WorkData {
  TA_FuncHandle *funcHandle;
  TA_ParamHolder *funcParams;
  TA_RetCode retCode;
  int startIdx;
  int endIdx;
  int outBegIdx;
  int outNBElement;
  unsigned int nbOutput;
  std::vector<double *> outReals;
  std::vector<int *> outIntegers;
  std::vector<void *> garbage;

  WorkData() {
    funcHandle = nullptr;
    funcParams = nullptr;
    retCode = TA_SUCCESS;
    startIdx = 0;
    endIdx = 0;
    nbOutput = 0;
    outBegIdx = 0;
    outNBElement = 0;
  }
} WorkData;

typedef struct AsyncWorkData {
  napi_ref cbref;
  napi_async_work worker;
  WorkData workData;
} AsyncWorkData;

static napi_status setArrayString(napi_env env, napi_value array, unsigned int index, const char *string) {
  napi_value value;

  CHECK(napi_create_string_utf8(env, string, NAPI_AUTO_LENGTH, &value));
  return napi_set_element(env, array, index, value);
}

static napi_status setArrayDouble(napi_env env, napi_value array, unsigned int index, double number) {
  napi_value value;

  CHECK(napi_create_double(env, number, &value));
  return napi_set_element(env, array, index, value);
}

static napi_status setArrayInt32(napi_env env, napi_value array, unsigned int index, int number) {
  napi_value value;

  CHECK(napi_create_int32(env, number, &value));
  return napi_set_element(env, array, index, value);
}

static napi_status setNamedPropertyString(napi_env env, napi_value object, const char *name, const char *string) {
  napi_value value;

  CHECK(napi_create_string_utf8(env, string, NAPI_AUTO_LENGTH, &value));
  return napi_set_named_property(env, object, name, value);
}

static napi_status setNamedPropertyDouble(napi_env env, napi_value object, const char *name, double number) {
  napi_value value;

  CHECK(napi_create_double(env, number, &value));
  return napi_set_named_property(env, object, name, value);
}

static napi_status setNamedPropertyInt32(napi_env env, napi_value object, const char *name, int number) {
  napi_value value;

  CHECK(napi_create_int32(env, number, &value));
  return napi_set_named_property(env, object, name, value);
}

static bool getNamedProperty(napi_env env, napi_value object, const char *name, napi_value *value) {
  bool hasProperty;

  CHECK(napi_has_named_property(env, object, name, &hasProperty));

  if (!hasProperty)
    return false;

  CHECK(napi_get_named_property(env, object, name, value));

  return true;
}

static bool getNamedPropertyString(napi_env env, napi_value object, const char *name, char* buf, size_t bufsize) {
  napi_value value;

  if (!getNamedProperty(env, object, name, &value))
    return false;

  CHECK(napi_get_value_string_utf8(env, value, buf, bufsize, nullptr));

  return true;
}

static bool getNamedPropertyDouble(napi_env env, napi_value object, const char *name, double *number) {
  napi_value value;

  if (!getNamedProperty(env, object, name, &value))
    return false;

  CHECK(napi_get_value_double(env, value, number));

  return true;
}

static bool getNamedPropertyInt32(napi_env env, napi_value object, const char *name, int *number) {
  napi_value value;

  if (!getNamedProperty(env, object, name, &value))
    return false;

  CHECK(napi_get_value_int32(env, value, number));

  return true;
}

static bool getNamedPropertyArray(napi_env env, napi_value object, const char *name, napi_value *array) {
  bool isArray;

  if (!getNamedProperty(env, object, name, array))
    return false;

  CHECK(napi_is_array(env, *array, &isArray));

  return isArray;
}

static double *getNamedPropertyDoubleArray(napi_env env, napi_value object, const char *name) {
  napi_value array, value;
  unsigned int length;
  double *numbers;

  if (!getNamedPropertyArray(env, object, name, &array))
    return nullptr;

  CHECK(napi_get_array_length(env, array, &length));

  numbers = (double *)malloc(sizeof(double) * length);

  for (unsigned int i = 0; i < length; ++i) {
    CHECK(napi_get_element(env, array, i, &value));
    CHECK(napi_get_value_double(env, value, &numbers[i]));
  }

  return numbers;
}

static int *getNamedPropertyInt32Array(napi_env env, napi_value object, const char *name) {
  napi_value array, value;
  unsigned int length;
  int *numbers;

  if (!getNamedPropertyArray(env, object, name, &array))
    return nullptr;

  CHECK(napi_get_array_length(env, array, &length));

  numbers = (int *)malloc(sizeof(int) * length);

  for (unsigned int i = 0; i < length; ++i) {
    CHECK(napi_get_element(env, array, i, &value));
    CHECK(napi_get_value_int32(env, value, &numbers[i]));
  }

  return numbers;
}

static napi_value getFunctionGroups(napi_env env, napi_callback_info info) {
  napi_value object, array;
  TA_StringTable *groupTable;
  TA_StringTable *funcTable;

  CHECK(napi_create_object(env, &object));

  if (TA_SUCCESS != TA_GroupTableAlloc(&groupTable))
    return object;

  for (unsigned int groupIndex = 0; groupIndex < groupTable->size; ++groupIndex) {
    if (TA_SUCCESS != TA_FuncTableAlloc(groupTable->string[groupIndex], &funcTable))
      continue;

    CHECK(napi_create_array_with_length(env, funcTable->size, &array));

    for (unsigned int funcIndex = 0; funcIndex < funcTable->size; ++funcIndex)
      CHECK(setArrayString(env, array, funcIndex, funcTable->string[funcIndex]));

    CHECK(napi_set_named_property(env, object, groupTable->string[groupIndex], array));

    TA_FuncTableFree(funcTable);
  }

  TA_GroupTableFree(groupTable);

  return object;
}

static napi_value getFunctions(napi_env env, napi_callback_info info) {
  napi_value array;
  TA_StringTable *groupTable;
  TA_StringTable *funcTable;
  int funcCount = 0;

  CHECK(napi_create_array(env, &array));

  if (TA_SUCCESS != TA_GroupTableAlloc(&groupTable))
    return array;

  for (unsigned int groupIndex = 0; groupIndex < groupTable->size; ++groupIndex) {
    if (TA_SUCCESS != TA_FuncTableAlloc(groupTable->string[groupIndex], &funcTable))
      continue;

    for (unsigned int funcIndex = 0; funcIndex < funcTable->size; ++funcIndex)
      CHECK(setArrayString(env, array, funcCount++, funcTable->string[funcIndex]));

    TA_FuncTableFree(funcTable);
  }

  TA_GroupTableFree(groupTable);

  return array;
}

static napi_value explain(napi_env env, napi_callback_info info) {
  size_t argc = 1;
  napi_value argv, jsthis, object, inputs, optInputs, outputs;
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

  CHECK(setNamedPropertyString(env, object, "name", funcInfo->name));
  CHECK(setNamedPropertyString(env, object, "group", funcInfo->group));
  CHECK(setNamedPropertyString(env, object, "hint", funcInfo->hint));

  CHECK(napi_create_array(env, &inputs));

  for (unsigned int i = 0; i < funcInfo->nbInput; ++i) {
    napi_value param, flags;
    unsigned int flagsCount = 0;

    TA_GetInputParameterInfo(funcInfo->handle, i, &inputParaminfo);

    CHECK(napi_create_object(env, &param));
    CHECK(napi_create_array(env, &flags));

    CHECK(setNamedPropertyString(env, param, "name", inputParaminfo->paramName));

    switch (inputParaminfo->type) {
      case TA_Input_Price:
        CHECK(setNamedPropertyString(env, param, "type", "price"));
        break;

      case TA_Input_Real:
        CHECK(setNamedPropertyString(env, param, "type", "real"));
        break;

      case TA_Input_Integer:
        CHECK(setNamedPropertyString(env, param, "type", "integer"));
        break;
    }

    if (inputParaminfo->flags & TA_IN_PRICE_OPEN)
      CHECK(setArrayString(env, flags, flagsCount++, "open"));

    if (inputParaminfo->flags & TA_IN_PRICE_HIGH)
      CHECK(setArrayString(env, flags, flagsCount++, "high"));

    if (inputParaminfo->flags & TA_IN_PRICE_LOW)
      CHECK(setArrayString(env, flags, flagsCount++, "low"));

    if (inputParaminfo->flags & TA_IN_PRICE_CLOSE)
      CHECK(setArrayString(env, flags, flagsCount++, "close"));

    if (inputParaminfo->flags & TA_IN_PRICE_VOLUME)
      CHECK(setArrayString(env, flags, flagsCount++, "volume"));

    if (inputParaminfo->flags & TA_IN_PRICE_OPENINTEREST)
      CHECK(setArrayString(env, flags, flagsCount++, "openinterest"));

    if (inputParaminfo->flags & TA_IN_PRICE_TIMESTAMP)
      CHECK(setArrayString(env, flags, flagsCount++, "timestamp"));

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

    CHECK(setNamedPropertyString(env, param, "name", optParaminfo->paramName));
    CHECK(setNamedPropertyString(env, param, "displayName", optParaminfo->displayName));
    CHECK(setNamedPropertyDouble(env, param, "defaultValue", optParaminfo->defaultValue));
    CHECK(setNamedPropertyString(env, param, "hint", optParaminfo->hint));

    switch (optParaminfo->type) {
      case TA_OptInput_RealRange:
        CHECK(setNamedPropertyString(env, param, "type", "real_range"));
        break;

      case TA_OptInput_RealList:
        CHECK(setNamedPropertyString(env, param, "type", "real_list"));
        break;

      case TA_OptInput_IntegerRange:
        CHECK(setNamedPropertyString(env, param, "type", "integer_range"));
        break;

      case TA_OptInput_IntegerList:
        CHECK(setNamedPropertyString(env, param, "type", "integer_list"));
        break;
    }

    if (optParaminfo->flags & TA_OPTIN_IS_PERCENT)
      CHECK(setArrayString(env, flags, flagsCount++, "percent"));

    if (optParaminfo->flags & TA_OPTIN_IS_DEGREE)
      CHECK(setArrayString(env, flags, flagsCount++, "degree"));

    if (optParaminfo->flags & TA_OPTIN_IS_CURRENCY)
      CHECK(setArrayString(env, flags, flagsCount++, "currency"));

    if (optParaminfo->flags & TA_OPTIN_ADVANCED)
      CHECK(setArrayString(env, flags, flagsCount++, "advanced"));

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

    CHECK(setNamedPropertyString(env, param, "name", outputParaminfo->paramName));

    switch (outputParaminfo->type) {
      case TA_Output_Real:
        CHECK(setNamedPropertyString(env, param, "type", "real"));
        break;
      case TA_Output_Integer:
        CHECK(setNamedPropertyString(env, param, "type", "integer"));
        break;
    }

    if (outputParaminfo->flags & TA_OUT_LINE)
      CHECK(setArrayString(env, flags, flagsCount++, "line"));

    if (outputParaminfo->flags & TA_OUT_DOT_LINE)
      CHECK(setArrayString(env, flags, flagsCount++, "line_dot"));

    if (outputParaminfo->flags & TA_OUT_DASH_LINE)
      CHECK(setArrayString(env, flags, flagsCount++, "line_dash"));

    if (outputParaminfo->flags & TA_OUT_DOT)
      CHECK(setArrayString(env, flags, flagsCount++, "dot"));

    if (outputParaminfo->flags & TA_OUT_HISTO)
      CHECK(setArrayString(env, flags, flagsCount++, "histogram"));

    if (outputParaminfo->flags & TA_OUT_PATTERN_BOOL)
      CHECK(setArrayString(env, flags, flagsCount++, "pattern_bool"));

    if (outputParaminfo->flags & TA_OUT_PATTERN_BULL_BEAR)
      CHECK(setArrayString(env, flags, flagsCount++, "pattern_bull_bear"));

    if (outputParaminfo->flags & TA_OUT_PATTERN_STRENGTH)
      CHECK(setArrayString(env, flags, flagsCount++, "pattern_strength"));

    if (outputParaminfo->flags & TA_OUT_POSITIVE)
      CHECK(setArrayString(env, flags, flagsCount++, "positive"));

    if (outputParaminfo->flags & TA_OUT_NEGATIVE)
      CHECK(setArrayString(env, flags, flagsCount++, "negative"));

    if (outputParaminfo->flags & TA_OUT_ZERO)
      CHECK(setArrayString(env, flags, flagsCount++, "zero"));

    if (outputParaminfo->flags & TA_OUT_UPPER_LIMIT)
      CHECK(setArrayString(env, flags, flagsCount++, "limit_upper"));

    if (outputParaminfo->flags & TA_OUT_LOWER_LIMIT)
      CHECK(setArrayString(env, flags, flagsCount++, "limit_lower"));

    CHECK(napi_set_named_property(env, param, "flags", flags));

    CHECK(napi_set_element(env, outputs, i, param));
  }

  CHECK(napi_set_named_property(env, object, "outputs", outputs));

  return object;
}

static napi_status createError(napi_env env, const char *errmsg, napi_value *error) {
  napi_value value;

  CHECK(napi_create_string_utf8(env, errmsg, NAPI_AUTO_LENGTH, &value));
  return napi_create_error(env, nullptr, value, error);
}

static napi_status createTAError(napi_env env, TA_RetCode retCode, napi_value *error) {
  napi_value errcode, errmsg;
  
  TA_RetCodeInfo retCodeInfo;
  TA_SetRetCodeInfo(retCode, &retCodeInfo);

  CHECK(napi_create_string_utf8(env, retCodeInfo.enumStr, NAPI_AUTO_LENGTH, &errcode));
  CHECK(napi_create_string_utf8(env, retCodeInfo.infoStr, NAPI_AUTO_LENGTH, &errmsg));
    
  return napi_create_error(env, errcode, errmsg, error);
}

static void freeWorkData(WorkData *workData) {
  if (workData->funcParams)
    TA_ParamHolderFree(workData->funcParams);

  if (workData->outReals.size() > 0) {
    for (auto iter = workData->outReals.begin(); iter != workData->outReals.end(); iter++)
      free(*iter);

    workData->outReals.clear();
  }

  if (workData->outIntegers.size() > 0) {
    for (auto iter = workData->outIntegers.begin(); iter != workData->outIntegers.end(); iter++)
      free(*iter);

    workData->outIntegers.clear();
  }

  if (workData->garbage.size() > 0) {
    for (auto iter = workData->garbage.begin(); iter != workData->garbage.end(); iter++)
      free(*iter);

    workData->garbage.clear();
  }
}

static bool parseWorkData(napi_env env, napi_value object, WorkData *workData, napi_value *error) {
  napi_value params;
  char funcName[64] = {0};
  double *open = nullptr;
  double *high = nullptr;
  double *low = nullptr;
  double *close = nullptr;
  double *volume = nullptr;
  double *openInterest = nullptr;
  double *inReal;
  int *inInteger;
  double optInReal;
  int optInInteger;
  double *outReal;
  int *outInteger;
  TA_RetCode retCode;
  const TA_FuncInfo *funcInfo;
  const TA_InputParameterInfo *inputParaminfo;
  const TA_OptInputParameterInfo *optParaminfo;
  const TA_OutputParameterInfo *outputParaminfo;

  if (!getNamedPropertyString(env, object, "name", funcName, sizeof(funcName))) {
    CHECK(createError(env, "Missing 'name' field", error));
    return false;
  }

  if (!getNamedPropertyInt32(env, object, "startIdx", &workData->startIdx)) {
    CHECK(createError(env, "Missing 'startIdx' field", error));
    return false;
  }

  if (!getNamedPropertyInt32(env, object, "endIdx", &workData->endIdx)) {
    CHECK(createError(env, "Missing 'endIdx' field", error));
    return false;
  }

  if (workData->startIdx < 0 || workData->endIdx < 0) {
    CHECK(createError(env, "Arguments 'startIdx' and 'endIdx' need to be positive", error));
    return false;
  }

  if (workData->startIdx > workData->endIdx) {
    CHECK(createError(env, "Argument 'startIdx' needs to be smaller than argument 'endIdx'", error));
    return false;
  }

  if (!getNamedProperty(env, object, "params", &params)) {
    CHECK(createError(env, "Missing 'params' field", error));
    return false;
  }

  if (TA_SUCCESS != (retCode = TA_GetFuncHandle(funcName, (const TA_FuncHandle **)&workData->funcHandle))) {
    CHECK(createTAError(env, retCode, error));
    return false;
  }

  if (TA_SUCCESS != (retCode = TA_GetFuncInfo(workData->funcHandle, &funcInfo))) {
    CHECK(createTAError(env, retCode, error));
    return false;
  }

  if (TA_SUCCESS != (retCode = TA_ParamHolderAlloc(workData->funcHandle, &workData->funcParams))) {
    CHECK(createTAError(env, retCode, error));
    return false;
  }

  workData->nbOutput = funcInfo->nbOutput;

  for (unsigned int i = 0; i < funcInfo->nbInput; ++i) {
    TA_GetInputParameterInfo(funcInfo->handle, i, &inputParaminfo);

    switch (inputParaminfo->type) {
      case TA_Input_Price:
        if (inputParaminfo->flags & TA_IN_PRICE_OPEN) {
          open = getNamedPropertyDoubleArray(env, params, "open");

          if (!open) {
            freeWorkData(workData);
            CHECK(createError(env, "Missing 'open' field", error));
            return false;
          }

          workData->garbage.push_back(open);
        }

        if (inputParaminfo->flags & TA_IN_PRICE_HIGH) {
          high = getNamedPropertyDoubleArray(env, params, "high");

          if (!high) {
            freeWorkData(workData);
            CHECK(createError(env, "Missing 'high' field", error));
            return false;
          }

          workData->garbage.push_back(high);
        }

        if (inputParaminfo->flags & TA_IN_PRICE_LOW) {
          low = getNamedPropertyDoubleArray(env, params, "low");

          if (!low) {
            freeWorkData(workData);
            CHECK(createError(env, "Missing 'low' field", error));
            return false;
          }

          workData->garbage.push_back(low);
        }

        if (inputParaminfo->flags & TA_IN_PRICE_CLOSE) {
          close = getNamedPropertyDoubleArray(env, params, "close");

          if (!close) {
            freeWorkData(workData);
            CHECK(createError(env, "Missing 'close' field", error));
            return false;
          }

          workData->garbage.push_back(close);
        }

        if (inputParaminfo->flags & TA_IN_PRICE_VOLUME) {
          volume = getNamedPropertyDoubleArray(env, params, "volume");

          if (!volume) {
            freeWorkData(workData);
            CHECK(createError(env, "Missing 'volume' field", error));
            return false;
          }

          workData->garbage.push_back(volume);
        }

        if (inputParaminfo->flags & TA_IN_PRICE_OPENINTEREST) {
          openInterest = getNamedPropertyDoubleArray(env, params, "openInterest");

          if (!openInterest) {
            freeWorkData(workData);
            CHECK(createError(env, "Missing 'openInterest' field", error));
            return false;
          }

          workData->garbage.push_back(openInterest);
        }

        if (TA_SUCCESS != (retCode = TA_SetInputParamPricePtr(workData->funcParams, i, open, high, low, close, volume, openInterest))) {
          freeWorkData(workData);
          CHECK(createTAError(env, retCode, error));
          return false;
        }

        break;

      case TA_Input_Real:
        inReal = getNamedPropertyDoubleArray(env, params, inputParaminfo->paramName);

        if (!inReal) {
          char errmsg[64] = {0};

          freeWorkData(workData);
          snprintf(errmsg, sizeof(errmsg), "Missing '%s' field", inputParaminfo->paramName);
          CHECK(createError(env, errmsg, error));
          return false;
        }

        workData->garbage.push_back(inReal);

        retCode = TA_SetInputParamRealPtr(workData->funcParams, i, inReal);

        if (TA_SUCCESS != retCode) {
          freeWorkData(workData);
          CHECK(createTAError(env, retCode, error));
          return false;
        }

        break;

      case TA_Input_Integer:
        inInteger = getNamedPropertyInt32Array(env, params, inputParaminfo->paramName);

        if (!inInteger) {
          char errmsg[64] = {0};

          freeWorkData(workData);
          snprintf(errmsg, sizeof(errmsg), "Missing '%s' field", inputParaminfo->paramName);
          CHECK(createError(env, errmsg, error));
          return false;
        }

        workData->garbage.push_back(inInteger);

        retCode = TA_SetInputParamIntegerPtr(workData->funcParams, i, inInteger);

        if (TA_SUCCESS != retCode) {
          freeWorkData(workData);
          CHECK(createTAError(env, retCode, error));
          return false;
        }

        break;
    }
  }

  for (unsigned int i = 0; i < funcInfo->nbOptInput; ++i) {
    TA_GetOptInputParameterInfo(funcInfo->handle, i, &optParaminfo);

    switch (optParaminfo->type) {
      case TA_OptInput_RealRange:
      case TA_OptInput_RealList:
        if (getNamedPropertyDouble(env, params, optParaminfo->paramName, &optInReal)) {
          if (TA_SUCCESS != (retCode = TA_SetOptInputParamReal(workData->funcParams, i, optInReal))) {
            freeWorkData(workData);
            CHECK(createTAError(env, retCode, error));
            return false;
          }
        }
        break;

      case TA_OptInput_IntegerRange:
      case TA_OptInput_IntegerList:
        if (getNamedPropertyInt32(env, params, optParaminfo->paramName, &optInInteger)) {
          if (TA_SUCCESS != (retCode = TA_SetOptInputParamInteger(workData->funcParams, i, optInInteger))) {
            freeWorkData(workData);
            CHECK(createTAError(env, retCode, error));
            return false;
          }
        }
        break;
    }
  }

  for (unsigned int i = 0; i < funcInfo->nbOutput; ++i) {
    TA_GetOutputParameterInfo(funcInfo->handle, i, &outputParaminfo);

    switch (outputParaminfo->type) {
      case TA_Output_Real:
        outReal = (double *)malloc(sizeof(double) * (workData->endIdx - workData->startIdx + 1));

        if (!outReal) {
          freeWorkData(workData);
          CHECK(createError(env, "Out of memory", error));
          return false;
        }

        memset(outReal, 0, sizeof(double) * (workData->endIdx - workData->startIdx + 1));
        workData->outReals.push_back(outReal);

        if (TA_SUCCESS != (retCode = TA_SetOutputParamRealPtr(workData->funcParams, i, outReal))) {
          freeWorkData(workData);
          CHECK(createTAError(env, retCode, error));
          return false;
        }

        break;

      case TA_Output_Integer:
        outInteger = (int *)malloc(sizeof(int) * (workData->endIdx - workData->startIdx + 1));

        if (!outInteger) {
          freeWorkData(workData);
          CHECK(createError(env, "Out of memory", error));
          return false;
        }

        memset(outInteger, 0, sizeof(int) * (workData->endIdx - workData->startIdx + 1));
        workData->outIntegers.push_back(outInteger);

        if (TA_SUCCESS != (retCode = TA_SetOutputParamIntegerPtr(workData->funcParams, i, outInteger))) {
          freeWorkData(workData);
          CHECK(createTAError(env, retCode, error));
          return false;
        }

        break;
    }
  }

  return true;
}

static bool generateResult(napi_env env, const WorkData *workData, napi_value *result) {
  napi_value object, array;
  int outRealIdx = 0;
  int outIntegerIdx = 0;
  double *outReal;
  int *outInteger;
  const TA_OutputParameterInfo *outputParaminfo;

  if (TA_SUCCESS != workData->retCode) {
    CHECK(createTAError(env, workData->retCode, result));
    return false;
  }

  CHECK(napi_create_object(env, result));
  CHECK(napi_create_object(env, &object));

  CHECK(setNamedPropertyInt32(env, *result, "begIndex", workData->outBegIdx));
  CHECK(setNamedPropertyInt32(env, *result, "nbElement", workData->outNBElement));

  for (unsigned int i = 0; i < workData->nbOutput; ++i) {
    TA_GetOutputParameterInfo(workData->funcHandle, i, &outputParaminfo);

    CHECK(napi_create_array_with_length(env, workData->outNBElement, &array));

    switch (outputParaminfo->type) {
      case TA_Output_Real:
        outReal = workData->outReals[outRealIdx++];

        for (int j = 0; j < workData->outNBElement; ++j)
          CHECK(setArrayDouble(env, array, j, outReal[j]));

        break;

      case TA_Output_Integer:
        outInteger = workData->outIntegers[outIntegerIdx++];

        for (int j = 0; j < workData->outNBElement; ++j)
          CHECK(setArrayInt32(env, array, j, outInteger[j]));

        break;
    }

    CHECK(napi_set_named_property(env, object, outputParaminfo->paramName, array));
  }

  CHECK(napi_set_named_property(env, *result, "results", object));

  return true;
}

static napi_value executeSync(napi_env env, napi_value jsthis, napi_value object) {
  napi_value result, error;
  WorkData workData;

  if (!parseWorkData(env, object, &workData, &error)) {
    CHECK(napi_throw(env, error));
    return jsthis;
  }

  workData.retCode = TA_CallFunc(workData.funcParams, workData.startIdx, workData.endIdx, &workData.outBegIdx, &workData.outNBElement);

  if (!generateResult(env, &workData, &result)) {
    freeWorkData(&workData);
    CHECK(napi_throw(env, result));
    return jsthis;
  }

  freeWorkData(&workData);
  return result;
}

static void executeAsyncCallback(napi_env env, void *data) {
  AsyncWorkData *asyncWorkData = (AsyncWorkData *)data;
  WorkData *workData = &asyncWorkData->workData;

  workData->retCode = TA_CallFunc(workData->funcParams, workData->startIdx, workData->endIdx, &workData->outBegIdx, &workData->outNBElement);
}

static void executeAsyncComplete(napi_env env, napi_status status, void *data) {
  AsyncWorkData *asyncWorkData = (AsyncWorkData *)data;
  napi_value undefined, callback, argv[2];

  CHECK(status);
  CHECK(napi_get_undefined(env, &undefined));
  CHECK(napi_get_reference_value(env, asyncWorkData->cbref, &callback));

  argv[0] = undefined;

  if (!generateResult(env, &asyncWorkData->workData, &argv[1])) {
    argv[0] = argv[1];
    argv[1] = undefined;
  }

  CHECK(napi_call_function(env, undefined, callback, 2, argv, nullptr));

  CHECK(napi_delete_reference(env, asyncWorkData->cbref));
  CHECK(napi_delete_async_work(env, asyncWorkData->worker));

  freeWorkData(&asyncWorkData->workData);
  delete asyncWorkData;
}

static napi_value executeAsync(napi_env env, napi_value object, napi_value callback) {
  napi_value undefined, name, argv[2];
  AsyncWorkData *asyncWorkData = new AsyncWorkData();

  CHECK(napi_get_undefined(env, &undefined));

  if (!asyncWorkData) {
    napi_throw_type_error(env, nullptr, "Out of memory");
    return undefined;
  }

  if (!parseWorkData(env, object, &asyncWorkData->workData, &argv[0])) {
    argv[1] = undefined;

    freeWorkData(&asyncWorkData->workData);
    delete asyncWorkData;

    CHECK(napi_call_function(env, undefined, callback, 2, argv, nullptr));
    return undefined;
  }

  CHECK(napi_create_reference(env, callback, 1, &asyncWorkData->cbref));
  CHECK(napi_create_string_utf8(env, "TA-Lib.Worker", NAPI_AUTO_LENGTH, &name));
  CHECK(napi_create_async_work(env, nullptr, name, executeAsyncCallback, executeAsyncComplete, asyncWorkData, &asyncWorkData->worker));
  CHECK(napi_queue_async_work(env, asyncWorkData->worker));

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
    : executeSync(env, jsthis, argv[0]);
}

static napi_value version(napi_env env, napi_callback_info info) {
  napi_value value;

  CHECK(napi_create_string_utf8(env, TA_GetVersionString(), NAPI_AUTO_LENGTH, &value));

  return value;
}

static napi_value init(napi_env env, napi_value exports) {
  TA_Initialize();

  napi_property_descriptor props[] = {
    DECLARE_NAPI_METHOD(getFunctionGroups),
    DECLARE_NAPI_METHOD(getFunctions),
    DECLARE_NAPI_METHOD(explain),
    DECLARE_NAPI_METHOD(execute),
    DECLARE_NAPI_METHOD(version),
  };
  CHECK(napi_define_properties(env, exports, arraysize(props), props));

  return exports;
}

NAPI_MODULE(NODE_GYP_MODULE_NAME, init)
