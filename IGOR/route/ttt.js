const WebSocket = require('ws');

// 서버 초기화
const server = new WebSocket.Server({ host: '0.0.0.0' ,port: 1029 });
console.log('WebSocket server running on ws://localhost:8080');

// 방 관리
const rooms = new Map();

server.on('connection', (socket) => {
  console.log('Client connected');

  let currentRoom = null;
  let userId = null;

  socket.on('message', (message) => {
    console.log(`Received: ${message}`);

    const match = message.toString().match(/^\[(.+?)\] (.+?): (.+)$/);
    if (match) {
      const roomId = match[1];
      const senderId = match[2];
      const content = match[3];

      // 방 초기화
      if (!rooms.has(roomId)) {
        rooms.set(roomId, {
          players: new Map(),
          connected: 0,
          board: Array(3).fill(0).map(() => Array(3).fill(0)), // 3x3 보드 초기화
          turn: 'X', // 게임 시작은 'X'부터
        });
      }

      const room = rooms.get(roomId);
      currentRoom = roomId;
      userId = senderId;

      // 플레이어 연결 처리
     // 플레이어 연결 처리
if (content === 'CONNECTED') {
  if (room.players.size < 2 && !room.players.has(userId)) {
    const symbol = room.players.size === 0 ? 'X' : 'O';
    room.players.set(userId, { socket, symbol });
    room.connected++;

    // 심볼 전달 로그 추가
    console.log(`[${roomId}] Server: Assigning symbol ${symbol} to ${userId}`);
    socket.send(`[${roomId}] Server: You are assigned ${symbol}`);

    if (room.connected === 2) {
      broadcast(roomId, `[${roomId}] Server: Game starts!`);
      broadcast(roomId, `[${roomId}] Server: Turn ${room.turn}`);
    } else {
      socket.send(`[${roomId}] Server: Waiting for second player...`);
    }
  } else {
    socket.send(`[${roomId}] Server: Room is full!`);
  }
}

if (content.startsWith('MOVE')) {
  const moveMatch = content.match(/MOVE (\d) (\d) (\w)/);
  if (moveMatch) {
    const row = parseInt(moveMatch[1]);
    const col = parseInt(moveMatch[2]);
    const moveSymbol = moveMatch[3];

    console.log(`Processing move: Row=${row}, Col=${col}, Symbol=${moveSymbol}`);

    // 수 처리 및 턴 전환
    if (room.board[row][col] === 0 && room.turn === moveSymbol) {
      room.board[row][col] = (moveSymbol === 'X') ? 1 : 2;

      // 턴 전환
      room.turn = (moveSymbol === 'X') ? 'O' : 'X';

      // MOVE 및 턴 정보 브로드캐스트
      broadcast(roomId, `[${roomId}] ${senderId}: MOVE ${row} ${col} ${moveSymbol}`);
      broadcast(roomId, `[${roomId}] Server: Turn ${room.turn}`); // 턴 전환 전송
    } else {
      socket.send(`[${roomId}] Server: Invalid move!`);
    }
  }
}

    }
  });

  socket.on('close', () => {
    console.log('Client disconnected');
    if (currentRoom && rooms.has(currentRoom)) {
      const room = rooms.get(currentRoom);
      room.players.delete(userId);
      if (room.players.size === 0) {
        rooms.delete(currentRoom); // 방 비우기
      }
    }
  });
});

// 브로드캐스트 함수
function broadcast(roomId, message) {
  const room = rooms.get(roomId);
  room.players.forEach((player) => {
    if (player.socket.readyState === WebSocket.OPEN) {
      player.socket.send(message);
    }
  });
}

// 승리 조건 확인
function checkWin(board, symbol) {
  const target = symbol === 'X' ? 1 : 2;

  // 행과 열 확인
  for (let i = 0; i < 3; i++) {
    if (board[i][0] === target && board[i][1] === target && board[i][2] === target) return 'WIN';
    if (board[0][i] === target && board[1][i] === target && board[2][i] === target) return 'WIN';
  }

  // 대각선 확인
  if (board[0][0] === target && board[1][1] === target && board[2][2] === target) return 'WIN';
  if (board[0][2] === target && board[1][1] === target && board[2][0] === target) return 'WIN';

  // 무승부 확인
  if (board.flat().every(cell => cell !== 0)) return 'DRAW';

  return null; // 계속 진행
}
