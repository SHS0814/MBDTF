const express = require('express');
const cors = require('cors');
const db = require('../db');

const app = express();

app.use(cors());

const router = express.Router();

router.use(cors()); // 라우터 수준에서 CORS 적용

// 로그인 경로
router.post('/login', async (req, res) => {
    const { username, password } = req.body;

    if (!username || !password) {
        return res.status(400).json({ success: false, message: 'Username and password required' });
    }

    try {
        // MySQL 쿼리 실행
        const query = 'SELECT * FROM users WHERE username = ?';
        const [results] = await db.query(query, [username]);

        // 사용자 존재 여부 확인
        if (results.length === 0 || results[0].password !== password) {
            return res.status(401).json({ success: false, message: 'Invalid username or password' });
        }

        res.json({ success: true, message: 'Login successful' });
    } catch (err) {
        console.error('Database query error:', err);
        res.status(500).json({ success: false, message: 'Database error' });
    }
});


// 닉네임 중복 확인 엔드포인트
router.get('/check-username', async (req, res) => {
    const { username } = req.query;

    if (!username) {
        return res.status(400).json({ success: false, message: 'Username is required' });
    }

    try {
        const query = 'SELECT COUNT(*) AS count FROM users WHERE username = ?';
        const [results] = await db.execute(query, [username]);

        if (results[0].count > 0) {
            return res.status(200).json({ success: false, message: 'Username already exists' });
        }

        res.status(200).json({ success: true, message: 'Username is available' });
    } catch (err) {
        console.error('Database query error:', err);
        res.status(500).json({ success: false, message: 'Database error' });
    }
});

// 계정 생성 엔드포인트
router.post('/register', async (req, res) => {
    const { username, password } = req.body;

    if (!username || !password) {
        return res.status(400).json({ success: false, message: 'Username and password are required' });
    }

    const connection = await db.getConnection(); // 트랜잭션 시작을 위해 커넥션 가져오기
    try {
        await connection.beginTransaction(); // 트랜잭션 시작

        const insertUserQuery = 'INSERT INTO users (username, password) VALUES (?, ?)';
        const [userResult] = await connection.execute(insertUserQuery, [username, password]);

        const userId = userResult.insertId; // 새로 생성된 사용자 ID 가져오기

        const insertScoresQuery = `
            INSERT INTO scores (user_id, username, game, high_score)
            VALUES (?, ?, '2048', 0), (?, ?, 'mine', 0), (?, ?, 'breakout', 0);
        `;
        await connection.execute(insertScoresQuery, [userId, username, userId, username, userId, username]);

        await connection.commit(); // 트랜잭션 커밋

        res.status(201).json({ success: true, message: 'User registered successfully' });
    } catch (err) {
        await connection.rollback(); // 오류 발생 시 트랜잭션 롤백

        if (err.code === 'ER_DUP_ENTRY') {
            return res.status(409).json({ success: false, message: 'Username already exists' });
        }
        console.error('Database query error:', err);
        res.status(500).json({ success: false, message: 'Database error' });
    } finally {
        connection.release(); // 연결 반환
    }
});


// Delete Account Endpoint
router.delete('/delete-account', async (req, res) => {
const username = req.query.username;

if (!username) {
    return res.status(400).json({ success: false, message: 'Username is required' });
}

try {
    // Delete user and related scores
    const deleteScoresQuery = 'DELETE FROM scores WHERE username = ?';
    const deleteUserQuery = 'DELETE FROM users WHERE username = ?';

    await db.execute(deleteScoresQuery, [username]);
    await db.execute(deleteUserQuery, [username]);

    res.status(200).json({ success: true, message: 'Account deleted successfully' });
} catch (err) {
    console.error('Database query error:', err);
    res.status(500).json({ success: false, message: 'Failed to delete account' });
}
});


module.exports = router;
