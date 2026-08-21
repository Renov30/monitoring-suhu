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

    if (!is_numeric($t) || !is_numeric($h)
        || $t < -40 || $t > 80
        || $h < 0 || $h > 100) {
        respond(400, ['ok' => false, 'error' => 'Data tidak valid']);
    }

    $stmt = $pdo->prepare(
        'INSERT INTO `' . DB_TABLE . '` (temperature, humidity) VALUES (:t, :h)'
    );
    $stmt->execute([':t' => (float)$t, ':h' => (float)$h]);

    respond(201, ['ok' => true, 'id' => (int)$pdo->lastInsertId()]);
}

// -----------------------------------------------------
// AMBIL RIWAYAT DATA (untuk grafik & tabel)
// -----------------------------------------------------

if ($method === 'GET') {

    if (($_GET['action'] ?? '') !== 'history') {
        respond(400, ['ok' => false, 'error' => 'Action tidak dikenal']);
    }

    $limit = isset($_GET['limit']) ? (int)$_GET['limit'] : 50;
    if ($limit < 1)   $limit = 50;
    if ($limit > 500) $limit = 500;

    $stmt = $pdo->prepare(
        'SELECT temperature, humidity, recorded_at
         FROM `' . DB_TABLE . '`
         ORDER BY id DESC
         LIMIT :lim'
    );
    $stmt->bindValue(':lim', $limit, PDO::PARAM_INT);
    $stmt->execute();

    $rows = $stmt->fetchAll(PDO::FETCH_ASSOC);

    respond(200, ['ok' => true, 'count' => count($rows), 'data' => $rows]);
}

respond(405, ['ok' => false, 'error' => 'Method tidak diizinkan']);
