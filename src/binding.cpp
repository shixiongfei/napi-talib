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

typedef struct WorkObject {
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

  WorkObject() {
    funcHandle = nullptr;
    funcParams = nullptr;
    retCode = TA_SUCCESS;
    startIdx = 0;
    endIdx = 0;
    nbOutput = 0;
    outBegIdx = 0;
    outNBElement = 0;
  }
} WorkObject;

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

static void freeWorkObject(WorkObject *workObject) {
  if (workObject->funcParams)
    TA_ParamHolderFree(workObject->funcParams);

  if (workObject->outReals.size() > 0) {
    for (auto iter = workObject->outReals.begin(); iter != workObject->outReals.end(); iter++)
      free(*iter);

    workObject->outReals.clear();
  }

  if (workObject->outIntegers.size() > 0) {
    for (auto iter = workObject->outIntegers.begin(); iter != workObject->outIntegers.end(); iter++)
      free(*iter);

    workObject->outIntegers.clear();
  }

  if (workObject->garbage.size() > 0) {
    for (auto iter = workObject->garbage.begin(); iter != workObject->garbage.end(); iter++)
      free(*iter);

    workObject->garbage.clear();
  }
}

static bool parseWorkObject(napi_env env, napi_value object, WorkObject *workObject, napi_value *error) {
  char funcName[64] = {0};
  double *open;
  double *high;
  double *low;
  double *close;
  double *volume;
  double *openInterest;
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

  if (!getNamedPropertyInt32(env, object, "startIdx", &workObject->startIdx)) {
    CHECK(createError(env, "Missing 'startIdx' field", error));
    return false;
  }

  if (!getNamedPropertyInt32(env, object, "endIdx", &workObject->endIdx)) {
    CHECK(createError(env, "Missing 'endIdx' field", error));
    return false;
  }

  if (workObject->startIdx < 0 || workObject->endIdx < 0) {
    CHECK(createError(env, "Arguments 'startIdx' and 'endIdx' need to be positive", error));
    return false;
  }

  if (workObject->startIdx > workObject->endIdx) {
    CHECK(createError(env, "Argument 'startIdx' needs to be smaller than argument 'endIdx'", error));
    return false;
  }

  if (TA_SUCCESS != (retCode = TA_GetFuncHandle(funcName, (const TA_FuncHandle **)&workObject->funcHandle))) {
    CHECK(createTAError(env, retCode, error));
    return false;
  }

  if (TA_SUCCESS != (retCode = TA_GetFuncInfo(workObject->funcHandle, &funcInfo))) {
    CHECK(createTAError(env, retCode, error));
    return false;
  }

  if (TA_SUCCESS != (retCode = TA_ParamHolderAlloc(workObject->funcHandle, &workObject->funcParams))) {
    CHECK(createTAError(env, retCode, error));
    return false;
  }

  workObject->nbOutput = funcInfo->nbOutput;

  for (unsigned int i = 0; i < funcInfo->nbInput; ++i) {
    TA_GetInputParameterInfo(funcInfo->handle, i, &inputParaminfo);

    switch (inputParaminfo->type) {
      case TA_Input_Price:
        if (inputParaminfo->flags & TA_IN_PRICE_OPEN) {
          open = getNamedPropertyDoubleArray(env, object, "open");

          if (!open) {
            freeWorkObject(workObject);
            CHECK(createError(env, "Missing 'open' field", error));
            return false;
          }

          workObject->garbage.push_back(open);
        }

        if (inputParaminfo->flags & TA_IN_PRICE_HIGH) {
          high = getNamedPropertyDoubleArray(env, object, "high");

          if (!high) {
            freeWorkObject(workObject);
            CHECK(createError(env, "Missing 'high' field", error));
            return false;
          }

          workObject->garbage.push_back(high);
        }

        if (inputParaminfo->flags & TA_IN_PRICE_LOW) {
          low = getNamedPropertyDoubleArray(env, object, "low");

          if (!low) {
            freeWorkObject(workObject);
            CHECK(createError(env, "Missing 'low' field", error));
            return false;
          }

          workObject->garbage.push_back(low);
        }

        if (inputParaminfo->flags & TA_IN_PRICE_CLOSE) {
          close = getNamedPropertyDoubleArray(env, object, "close");

          if (!close) {
            freeWorkObject(workObject);
            CHECK(createError(env, "Missing 'close' field", error));
            return false;
          }

          workObject->garbage.push_back(close);
        }

        if (inputParaminfo->flags & TA_IN_PRICE_VOLUME) {
          volume = getNamedPropertyDoubleArray(env, object, "volume");

          if (!volume) {
            freeWorkObject(workObject);
            CHECK(createError(env, "Missing 'volume' field", error));
            return false;
          }

          workObject->garbage.push_back(volume);
        }

        if (inputParaminfo->flags & TA_IN_PRICE_OPENINTEREST) {
          openInterest = getNamedPropertyDoubleArray(env, object, "openInterest");

          if (!openInterest) {
            freeWorkObject(workObject);
            CHECK(createError(env, "Missing 'openInterest' field", error));
            return false;
          }

          workObject->garbage.push_back(openInterest);
        }

        if (TA_SUCCESS != (retCode = TA_SetInputParamPricePtr(workObject->funcParams, i, open, high, low, close, volume, openInterest))) {
          freeWorkObject(workObject);
          CHECK(createTAError(env, retCode, error));
          return false;
        }

        break;

      case TA_Input_Real:
        inReal = getNamedPropertyDoubleArray(env, object, inputParaminfo->paramName);

        if (!inReal) {
          char errmsg[64] = {0};

          freeWorkObject(workObject);
          snprintf(errmsg, sizeof(errmsg), "Missing '%s' field", inputParaminfo->paramName);
          CHECK(createError(env, errmsg, error));
          return false;
        }

        workObject->garbage.push_back(inReal);

        retCode = TA_SetInputParamRealPtr(workObject->funcParams, i, inReal);

        if (TA_SUCCESS != retCode) {
          freeWorkObject(workObject);
          CHECK(createTAError(env, retCode, error));
          return false;
        }

        break;

      case TA_Input_Integer:
        inInteger = getNamedPropertyInt32Array(env, object, inputParaminfo->paramName);

        if (!inInteger) {
          char errmsg[64] = {0};

          freeWorkObject(workObject);
          snprintf(errmsg, sizeof(errmsg), "Missing '%s' field", inputParaminfo->paramName);
          CHECK(createError(env, errmsg, error));
          return false;
        }

        workObject->garbage.push_back(inInteger);

        retCode = TA_SetInputParamIntegerPtr(workObject->funcParams, i, inInteger);

        if (TA_SUCCESS != retCode) {
          freeWorkObject(workObject);
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
        if (getNamedPropertyDouble(env, object, optParaminfo->paramName, &optInReal)) {
          if (TA_SUCCESS != (retCode = TA_SetOptInputParamReal(workObject->funcParams, i, optInReal))) {
            freeWorkObject(workObject);
            CHECK(createTAError(env, retCode, error));
            return false;
          }
        }
        break;

      case TA_OptInput_IntegerRange:
      case TA_OptInput_IntegerList:
        if (getNamedPropertyInt32(env, object, optParaminfo->paramName, &optInInteger)) {
          if (TA_SUCCESS != (retCode = TA_SetOptInputParamInteger(workObject->funcParams, i, optInInteger))) {
            freeWorkObject(workObject);
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
        outReal = (double *)malloc(sizeof(double) * (workObject->endIdx - workObject->startIdx + 1));

        if (!outReal) {
          freeWorkObject(workObject);
          CHECK(createError(env, "Out of memory", error));
          return false;
        }

        memset(outReal, 0, sizeof(double) * (workObject->endIdx - workObject->startIdx + 1));
        workObject->outReals.push_back(outReal);

        if (TA_SUCCESS != (retCode = TA_SetOutputParamRealPtr(workObject->funcParams, i, outReal))) {
          freeWorkObject(workObject);
          CHECK(createTAError(env, retCode, error));
          return false;
        }

        break;

      case TA_Output_Integer:
        outInteger = (int *)malloc(sizeof(int) * (workObject->endIdx - workObject->startIdx + 1));

        if (!outInteger) {
          freeWorkObject(workObject);
          CHECK(createError(env, "Out of memory", error));
          return false;
        }

        memset(outInteger, 0, sizeof(int) * (workObject->endIdx - workObject->startIdx + 1));
        workObject->outIntegers.push_back(outInteger);

        if (TA_SUCCESS != (retCode = TA_SetOutputParamIntegerPtr(workObject->funcParams, i, outInteger))) {
          freeWorkObject(workObject);
          CHECK(createTAError(env, retCode, error));
          return false;
        }

        break;
    }
  }

  return true;
}

static bool generateResult(napi_env env, const WorkObject *workObject, napi_value *result) {
  napi_value object, array;
  int outRealIdx = 0;
  int outIntegerIdx = 0;
  double *outReal;
  int *outInteger;
  const TA_OutputParameterInfo *outputParaminfo;

  if (TA_SUCCESS != workObject->retCode) {
    CHECK(createTAError(env, workObject->retCode, result));
    return false;
  }

  CHECK(napi_create_object(env, result));
  CHECK(napi_create_object(env, &object));

  CHECK(setNamedPropertyInt32(env, *result, "begIndex", workObject->outBegIdx));
  CHECK(setNamedPropertyInt32(env, *result, "nbElement", workObject->outNBElement));

  for (unsigned int i = 0; i < workObject->nbOutput; ++i) {
    TA_GetOutputParameterInfo(workObject->funcHandle, i, &outputParaminfo);

    CHECK(napi_create_array_with_length(env, workObject->outNBElement, &array));

    switch (outputParaminfo->type) {
      case TA_Output_Real:
        outReal = workObject->outReals[outRealIdx++];

        for (int j = 0; j < workObject->outNBElement; ++i)
          CHECK(setArrayDouble(env, array, j, outReal[j]));

        break;

      case TA_Output_Integer:
        outInteger = workObject->outIntegers[outIntegerIdx++];

        for (int j = 0; j < workObject->outNBElement; ++i)
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
  WorkObject workObject;

  if (!parseWorkObject(env, object, &workObject, &error)) {
    CHECK(napi_throw(env, error));
    return jsthis;
  }

  workObject.retCode = TA_CallFunc(workObject.funcParams, workObject.startIdx, workObject.endIdx, &workObject.outBegIdx, &workObject.outNBElement);

  if (!generateResult(env, &workObject, &result)) {
    freeWorkObject(&workObject);
    CHECK(napi_throw(env, result));
    return jsthis;
  }

  freeWorkObject(&workObject);
  return result;
}

static napi_value executeAsync(napi_env env, napi_value object, napi_value callback) {
  napi_value undefined, argv[2];
  WorkObject workObject;

  CHECK(napi_get_undefined(env, &undefined));

  if (!parseWorkObject(env, object, &workObject, &argv[0])) {
    argv[1] = undefined;
    CHECK(napi_call_function(env, undefined, callback, 2, argv, nullptr));
    return undefined;
  }

  argv[0] = undefined;
  argv[1] = undefined;
  
  freeWorkObject(&workObject);
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
