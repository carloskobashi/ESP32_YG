#include "sdm630.h"
#include "config.h"
#include <math.h>
#include <string.h>

static float decodeFloatBE(const uint16_t *buf, uint16_t i) {
  uint32_t raw = ((uint32_t)buf[i] << 16) | buf[i + 1];
  float value;
  memcpy(&value, &raw, sizeof(value));
  return value;
}

float SDM630::f(const uint16_t *buf, uint16_t index) const { return decodeFloatBE(buf, index); }

bool SDM630::block(uint16_t start, uint16_t count, uint16_t *dst) {
  return _bus.readInputRegisters(SDM_SLAVE_ID, start, count, dst);
}

// Register offsets are relative to Modbus register 30001.
// The meter uses function 04 and IEEE-754 FLOAT32, high word first.
bool SDM630::read(SDM630Data &d) {
  d = SDM630Data();
  memset(r, 0, sizeof(r));

  // 30001..30079
  if (!block(0, 79, r)) return false;

  // 30081..30111 -> stored locally in a temporary contiguous buffer.
  uint16_t b1[31];
  if (!block(80, 31, b1)) return false;

  // 30201..30225
  uint16_t b2[25];
  if (!block(200, 25, b2)) return false;

  // 30235..30269
  uint16_t b3[36];
  if (!block(234, 36, b3)) return false;

  // 30335..30395
  uint16_t b4[61];
  if (!block(334, 61, b4)) return false;

  d.online = true;

  // Helper by absolute offset, using the appropriate block.
  auto absF = [&](uint16_t off) -> float {
    if (off <= 78) return f(r, off);
    if (off >= 80 && off <= 110) return f(b1, off - 80);
    if (off >= 200 && off <= 224) return f(b2, off - 200);
    if (off >= 234 && off <= 269) return f(b3, off - 234);
    if (off >= 334 && off <= 395) return f(b4, off - 334);
    return NAN;
  };

  d.voltageA = absF(0); d.voltageB = absF(2); d.voltageC = absF(4);
  d.currentA = absF(6); d.currentB = absF(8); d.currentC = absF(10);
  d.powerA = absF(12); d.powerB = absF(14); d.powerC = absF(16);
  d.apparentA = absF(18); d.apparentB = absF(20); d.apparentC = absF(22);
  d.reactiveA = absF(24); d.reactiveB = absF(26); d.reactiveC = absF(28);
  d.pfA = absF(30); d.pfB = absF(32); d.pfC = absF(34);
  d.angleA = absF(36); d.angleB = absF(38); d.angleC = absF(40);
  d.voltageA = absF(0); // explicit for clarity
  d.frequencyHz = absF(70);
  d.importKWh = absF(72); d.exportKWh = absF(74);
  d.importKVARh = absF(76); d.exportKVARh = absF(78);
  d.totalKVAh = absF(80); d.ampHours = absF(82);
  d.powerDemandW = absF(84); d.maxPowerDemandW = absF(86);
  d.vaDemand = absF(100); d.maxVaDemand = absF(102);
  d.neutralDemandA = absF(104); d.maxNeutralDemandA = absF(106);
  d.reactiveDemandVar = absF(108); d.maxReactiveDemandVar = absF(110);

  d.lineVoltageAB = absF(200); d.lineVoltageBC = absF(202); d.lineVoltageCA = absF(204); d.lineVoltageAvg = absF(206);
  d.currentNeutral = absF(224);

  d.thdVoltageA = absF(234); d.thdVoltageB = absF(236); d.thdVoltageC = absF(238);
  d.thdCurrentA = absF(240); d.thdCurrentB = absF(242); d.thdCurrentC = absF(244);
  d.thdVoltageAvg = absF(248); d.thdCurrentAvg = absF(250);
  d.pfTotal = absF(254);
  d.currentDemandA = absF(258); d.currentDemandB = absF(260); d.currentDemandC = absF(262);
  d.maxCurrentDemandA = absF(264); d.maxCurrentDemandB = absF(266); d.maxCurrentDemandC = absF(268);

  d.thdVoltageAB = absF(334); d.thdVoltageBC = absF(336); d.thdVoltageCA = absF(338); d.thdVoltageLineAvg = absF(340);
  d.totalKWh = absF(342); d.totalKVARh = absF(344);
  d.importKWhA = absF(346); d.importKWhB = absF(348); d.importKWhC = absF(350);
  d.exportKWhA = absF(352); d.exportKWhB = absF(354); d.exportKWhC = absF(356);
  d.totalKWhA = absF(358); d.totalKWhB = absF(360); d.totalKWhC = absF(362);
  d.importKVARhA = absF(364); d.importKVARhB = absF(366); d.importKVARhC = absF(368);
  d.exportKVARhA = absF(370); d.exportKVARhB = absF(372); d.exportKVARhC = absF(374);
  d.totalKVARhA = absF(376); d.totalKVARhB = absF(378); d.totalKVARhC = absF(380);
  d.resetTotalKWh = absF(384); d.resetTotalKVARh = absF(386);
  d.resetImportKWh = absF(388); d.resetExportKWh = absF(390);
  d.resetImportKVARh = absF(392); d.resetExportKVARh = absF(394);

  d.currentAvg = absF(46);
  d.currentSum = absF(48);
  d.powerTotal = absF(52);
  d.apparentTotal = absF(56);
  d.reactiveTotal = absF(60);
  d.pfTotal = absF(62);
  d.angleTotal = absF(66);
  d.valid = validate(d);
  return d.valid;
}

bool SDM630::validate(SDM630Data &d) {
  uint32_t q = 0;
  auto finite = [](float x) { return isfinite(x); };

  if (!finite(d.voltageA) || d.voltageA < 0 || d.voltageA > MAX_VOLTAGE_V) q |= 1UL << 0;
  if (!finite(d.voltageB) || d.voltageB < 0 || d.voltageB > MAX_VOLTAGE_V) q |= 1UL << 1;
  if (!finite(d.voltageC) || d.voltageC < 0 || d.voltageC > MAX_VOLTAGE_V) q |= 1UL << 2;
  if (!finite(d.currentA) || fabs(d.currentA) > MAX_CURRENT_A) q |= 1UL << 3;
  if (!finite(d.currentB) || fabs(d.currentB) > MAX_CURRENT_A) q |= 1UL << 4;
  if (!finite(d.currentC) || fabs(d.currentC) > MAX_CURRENT_A) q |= 1UL << 5;
  if (!finite(d.frequencyHz) || d.frequencyHz < MIN_FREQUENCY_HZ || d.frequencyHz > MAX_FREQUENCY_HZ) q |= 1UL << 6;
  if (!finite(d.powerTotal) || fabs(d.powerTotal) > MAX_POWER_W) q |= 1UL << 7;
  if (!finite(d.reactiveTotal) || fabs(d.reactiveTotal) > MAX_POWER_W) q |= 1UL << 8;
  if (!finite(d.apparentTotal) || fabs(d.apparentTotal) > MAX_POWER_W) q |= 1UL << 9;
  if (!finite(d.pfTotal) || fabs(d.pfTotal) > 1.05f) q |= 1UL << 10;
  if (!finite(d.importKWh) || d.importKWh < 0 || d.importKWh > MAX_ENERGY_KWH) q |= 1UL << 11;
  if (!finite(d.exportKWh) || d.exportKWh < 0 || d.exportKWh > MAX_ENERGY_KWH) q |= 1UL << 12;
  if (!finite(d.thdCurrentA) || d.thdCurrentA < 0 || d.thdCurrentA > MAX_THD_PERCENT) q |= 1UL << 13;
  if (!finite(d.thdCurrentB) || d.thdCurrentB < 0 || d.thdCurrentB > MAX_THD_PERCENT) q |= 1UL << 14;
  if (!finite(d.thdCurrentC) || d.thdCurrentC < 0 || d.thdCurrentC > MAX_THD_PERCENT) q |= 1UL << 15;

  d.qualityFlags = q;
  return q == 0;
}
