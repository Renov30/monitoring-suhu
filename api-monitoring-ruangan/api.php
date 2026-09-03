<?php
// =====================================================
// API MONITORING SUHU & KELEMBAPAN RUANGAN
// =====================================================
// POST /api.php                      -> simpan data sensor
//     body JSON: {"temperature":27.5,"humidity":60.0}
// GET  /api.php?action=history&limit=50 -> ambil riwayat
// =====================================================

header('Access-Control-Allow-Origin: *');
header('Content-Type: application/json; charset=utf-8');

require __DIR__ . '/config.php';

function respond(int $code, array $data): void {
    http_response_code($code);
    echo json_encode($data);
    exit;
}

$method = $_SERVER['REQUEST_METHOD'];

if ($method === 'OPTIONS') {
    header('Access-Control-Allow-Methods: GET, POST, OPTIONS');
    header('Access-Control-Allow-Headers: Content-Type');
    http_response_code(204);
    exit;
}

try {
    $pdo = new PDO(
        'mysql:host=' . DB_HOST . ';dbname=' . DB_NAME . ';charset=utf8mb4',
        DB_USER,
        DB_PASS,
        [PDO::ATTR_ERRMODE => PDO::ERRMODE_EXCEPTION]
    );
} catch (PDOException $e) {
    respond(500, ['ok' => false, 'error' => 'Koneksi database gagal']);
}

// -----------------------------------------------------
// SIMPAN DATA SENSOR (dari ESP32)
// -----------------------------------------------------

if ($method === 'POST') {

    $input = json_decode(file_get_contents('php://input'), true);

    $t = $input['temperature'] ?? null;
    $h = $input['humidity'] ?? null;
    $d = $input['device'] ?? null;

    if (!is_numeric($t) || !is_numeric($h)
        || $t < -40 || $t > 80
        || $h < 0 || $h > 100
        || !is_string($d) || strlen($d) < 1 || strlen($d) > 50) {
        respond(400, ['ok' => false, 'error' => 'Data tidak valid']);
    }

    $stmt = $pdo->prepare(
        'INSERT INTO `' . DB_TABLE . '` (device, temperature, humidity) VALUES (:d, :t, :h)'
    );
    $stmt->execute([':d' => $d, ':t' => (float)$t, ':h' => (float)$h]);

    respond(201, ['ok' => true, 'id' => (int)$pdo->lastInsertId()]);
}

// -----------------------------------------------------
// AMBIL RIWAYAT DATA (untuk grafik & tabel)
// -----------------------------------------------------

if ($method === 'GET') {

    if (($_GET['action'] ?? '') !== 'history') {
        respond(400, ['ok' => false, 'error' => 'Action tidak dikenal']);
    }

    $device = trim($_GET['device'] ?? '');

    $date = $_GET['date'] ?? '';

    if (preg_match('/^\d{4}-\d{2}-\d{2}$/', $date) === 1) {

        // -------------------------------------------------
        // MODE PER TANGGAL: ambil semua data 1 hari penuh
        // -------------------------------------------------

        $endTs = strtotime($date . ' +1 day');

        if ($endTs === false) {
            respond(400, ['ok' => false, 'error' => 'Tanggal tidak valid']);
        }

        $end = date('Y-m-d', $endTs);

        $sql = 'SELECT device, temperature, humidity, recorded_at
                FROM `' . DB_TABLE . '`
                WHERE recorded_at >= :start AND recorded_at < :end';

        $bind = [':start' => $date . ' 00:00:00', ':end' => $end . ' 00:00:00'];

        if ($device !== '') {
            $sql .= ' AND device = :dev';
            $bind[':dev'] = $device;
        }

        $sql .= ' ORDER BY recorded_at ASC';

        $stmt = $pdo->prepare($sql);
        $stmt->execute($bind);

    } else {

        // -------------------------------------------------
        // MODE LAMA: N data terakhir
        // -------------------------------------------------

        $limit = isset($_GET['limit']) ? (int)$_GET['limit'] : 50;
        if ($limit < 1)   $limit = 50;
        if ($limit > 500) $limit = 500;

        $sql = 'SELECT device, temperature, humidity, recorded_at
                FROM `' . DB_TABLE . '`';

        $bind = [];

        if ($device !== '') {
            $sql .= ' WHERE device = :dev';
            $bind[':dev'] = $device;
        }

        $sql .= ' ORDER BY id DESC LIMIT :lim';

        $stmt = $pdo->prepare($sql);

        foreach ($bind as $key => $value) {
            $stmt->bindValue($key, $value, PDO::PARAM_STR);
        }
        $stmt->bindValue(':lim', $limit, PDO::PARAM_INT);
        $stmt->execute();
    }

    $rows = $stmt->fetchAll(PDO::FETCH_ASSOC);

    respond(200, ['ok' => true, 'count' => count($rows), 'data' => $rows]);
}

respond(405, ['ok' => false, 'error' => 'Method tidak diizinkan']);
