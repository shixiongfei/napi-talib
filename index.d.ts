/*
 * index.d.ts
 *
 * Copyright (c) 2024 Xiongfei Shi
 *
 * Author: Xiongfei Shi <xiongfei.shi(a)icloud.com>
 * License: Apache-2.0
 *
 * https://github.com/shixiongfei/napi-talib
 */

/** Get functions including grouping */
export declare function getFunctionGroups(): Record<string, string[]>;

/** Get functions */
export declare function getFunctions(): string[];
