/*
 * index.js
 *
 * Copyright (c) 2024-2025 Xiongfei Shi
 *
 * Author: Xiongfei Shi <xiongfei.shi(a)icloud.com>
 * License: Apache-2.0
 *
 * https://github.com/shixiongfei/napi-talib
 */

export * from "./types.js";

import { dlopen } from "node:process";
import { constants } from "node:os";
import { dirname, join } from "node:path";
import { fileURLToPath } from "node:url";
import * as types from "./types.js";

const binding = () => {
  const dir = dirname(fileURLToPath(import.meta.url));
  const module = { exports: {} };

  try {
    dlopen(
      module,
      join(dir, "./build/Release/napi_talib.node"),
      constants.dlopen.RTLD_LAZY
    );
  } catch {
    try {
      dlopen(
        module,
        join(dir, "./build/Debug/napi_talib.node"),
        constants.dlopen.RTLD_LAZY
      );
    } catch {
      throw new Error("Cannot find module 'napi_talib.node'");
    }
  }

  return module;
};

const native = binding().exports;

export const getFunctionGroups = native.getFunctionGroups;
export const getFunctions = native.getFunctions;
export const setUnstablePeriod = native.setUnstablePeriod;
export const setCompatibility = native.setCompatibility;
export const explain = native.explain;
export const execute = native.execute;
export const version = native.version;

export default Object.assign(native, types);
