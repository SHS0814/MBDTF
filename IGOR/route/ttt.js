const WebSocket = require('ws');

// 서버 초기화
const server = new WebSocket.Server({ port: 8080 });

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
          board: Array(3).fill(0).map(() => Array(3).fill(0)),
          turn: 'X', // 시작은 X
        });
      }

      const room = rooms.get(roomId);
      currentRoom = roomId;
      userId = senderId;

      // 플레이어 연결 처리
      if (content === 'CONNECTED') {
        if (room.players.size < 2 && !room.players.has(userId)) {
          const symbol = room.players.size === 0 ? 'X' : 'O';
          room.players.set(userId, { socket, symbol });
          room.connected++;

          socket.send(`[${roomId}] Server: You are assigned ${symbol}`);

          if (room.connected === 2) {
            // 두 명이 모이면 게임 시작 신호 전송
            broadcast(roomId, `[${roomId}] Server: Game starts!`);
          } else {
            socket.send(`[${roomId}] Server: Waiting for second player...`);
          }
        } else {
          socket.send(`[${roomId}] Server: Room is full!`);
        }
      }

      // 수 처리
      if (content.startsWith('MOVE')) {
        const moveMatch = content.match(/MOVE (\d) (\d) (\w)/);
        if (moveMatch) {
          const row = parseInt(moveMatch[1]);
          const col = parseInt(moveMatch[2]);
          const moveSymbol = moveMatch[3];

          // 턴 및 유효성 검증
          if (room.board[row][col] === 0 && room.turn === moveSymbol) {
            room.board[row][col] = (moveSymbol === 'X') ? 1 : 2;
            room.turn = (moveSymbol === 'X') ? 'O' : 'X';

            // 브로드캐스트
            broadcast(roomId, `[${roomId}] ${senderId}: MOVE ${row} ${col} ${moveSymbol}`);
          } else {
            socket.send(`[${roomId}] Server: Invalid move!`);
          }
        }
      }
    }
  });

  socket.on('close', () => {
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

// 서버 시작
console.log('WebSocket server running on ws://localhost:8080');
