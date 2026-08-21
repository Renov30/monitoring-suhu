/*
 Monitoring Suhu & Kelembapan Ruangan
 MIGRASI: tambah kolom `device` (multi ruangan/perangkat)
 Target Schema : monitoring (MariaDB 12.x)
 Jalankan SEKALI pada database yang sudah ada.
*/

SET NAMES utf8mb4;

ALTER TABLE `monitoring_ruangan`
  ADD COLUMN `device` varchar(50) CHARACTER SET utf8mb4 COLLATE utf8mb4_uca1400_ai_ci NOT NULL COMMENT 'Identitas perangkat/ruangan' AFTER `id`,
  ADD INDEX `idx_device_recorded`(`device` ASC, `recorded_at` ASC) USING BTREE;

-- Opsional: isi identitas untuk data lama (jika ada), contoh:
-- UPDATE `monitoring_ruangan` SET `device` = 'wt32-ruang-a' WHERE `device` = '';
