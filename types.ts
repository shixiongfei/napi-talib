/*
 * types.ts
 *
 * Copyright (c) 2024 Xiongfei Shi
 *
 * Author: Xiongfei Shi <xiongfei.shi(a)icloud.com>
 * License: Apache-2.0
 *
 * https://github.com/shixiongfei/napi-talib
 */

export enum MAType {
  SMA,
  EMA,
  WMA,
  DEMA,
  TEMA,
  TRIMA,
  KAMA,
  MAMA,
  T3,
}

export enum FuncUnstId {
  NONE = -1,
  ADX,
  ADXR,
  ATR,
  CMO,
  DX,
  EMA,
  HT_DCPERIOD,
  HT_DCPHASE,
  HT_PHASOR,
  HT_SINE,
  HT_TRENDLINE,
  HT_TRENDMODE,
  IMI,
  KAMA,
  MAMA,
  MFI,
  MINUS_DI,
  MINUS_DM,
  NATR,
  PLUS_DI,
  PLUS_DM,
  RSI,
  STOCHRSI,
  T3,
  ALL,
}

export enum Compatibility {
  DEFAULT,
  METASTOCK,
}
