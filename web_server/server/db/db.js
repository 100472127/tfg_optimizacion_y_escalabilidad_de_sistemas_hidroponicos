import { createPool } from 'mysql2/promise';

const pool = createPool({
    host: 'localhost',
    user: 'root',
    password: '123456',
    database: 'sensor_data_db',
    waitForConnections: true,
    connectionLimit: 10,
    queueLimit: 0
});

export default pool;