-- Crear la base de datos
CREATE DATABASE IF NOT EXISTS sensor_data_db;
USE sensor_data_db;

-- Crear la tabla de lecturas
CREATE TABLE IF NOT EXISTS sensor_readings (
    id INT AUTO_INCREMENT PRIMARY KEY,
    phRead DECIMAL(5,2),
    waterQualityRead DECIMAL(6, 2),
    tempRead DECIMAL(5,2),
    humidityRead DECIMAL(5,2),
    lightResistorRead DECIMAL(8,2),
    lightSensorRead DECIMAL(8,2),
    inserted_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    deleted_at DATETIME DEFAULT NULL
);

-- Trigger para transformar DELETE en UPDATE
DELIMITER //
CREATE TRIGGER before_delete_sensor_readings
BEFORE DELETE ON sensor_readings
FOR EACH ROW
BEGIN
    -- Actualizar el campo deleted_at en lugar de borrar
    UPDATE sensor_readings
    SET deleted_at = NOW()
    WHERE id = OLD.id;

    -- Cancelar la operación DELETE
    SIGNAL SQLSTATE '45000'
    SET MESSAGE_TEXT = 'Eliminación transformada en marcado como eliminado.';
END;
//
DELIMITER ;