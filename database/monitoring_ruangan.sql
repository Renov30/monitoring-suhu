/*
 Monitoring Suhu & Kelembapan Ruangan
 Target Schema : monitoring (MariaDB 12.x)
 Perangkat     : WT32-ETH01 + DHT11
 Interval      : 1 menit (~1440 baris/hari)
*/

SET NAMES utf8mb4;
SET FOREIGN_KEY_CHECKS = 0;

-- ----------------------------
-- Table structure for monitoring_ruangan
-- ----------------------------
DROP TABLE IF EXISTS `monitoring_ruangan`;
CREATE TABLE `monitoring_ruangan`  (
  `id` bigint(20) UNSIGNED NOT NULL AUTO_INCREMENT,
  `temperature` float NOT NULL COMMENT 'Suhu (°C)',
  `humidity` float NOT NULL COMMENT 'Kelembapan relatif (%)',
  `recorded_at` datetime NOT NULL DEFAULT CURRENT_TIMESTAMP COMMENT 'Waktu simpan (jam server)',
  PRIMARY KEY (`id`) USING BTREE,
  INDEX `idx_recorded_at`(`recorded_at` ASC) USING BTREE
) ENGINE = InnoDB CHARACTER SET = utf8mb4 COLLATE = utf8mb4_uca1400_ai_ci ROW_FORMAT = Dynamic;

SET FOREIGN_KEY_CHECKS = 1;
