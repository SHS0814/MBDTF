const WebSocket = require('ws');

// 서버 설정
const server = new WebSocket.Server({ port: 8080 });

// 방 관리 객체
const rooms = new Map();

// 클라이언트 연결 처리
server.on('connection', (socket) => {
  console.log('Client connected');

  let currentRoom = null; // 클라이언트의 현재 방 ID
  let userId = null;      // 클라이언트의 사용자 ID

  // 메시지 수신 처리
  socket.on('message', (message) => {
    console.log(`Received: ${message}`);

    // 방 ID, 사용자 ID, 메시지 분리
    const match = message.toString().match(/^\[(.+?)\] (.+?): (.+)$/);
    if (match) {
      const roomId = match[1];   // 방 ID
      const senderId = match[2]; // 보낸 사용자 ID
      const msgContent = match[3]; // 메시지 내용

      // 방 및 사용자 설정
      if (!rooms.has(roomId)) {
        rooms.set(roomId, new Set());
      }

      // 방 입장 처리
      if (!currentRoom) {
        currentRoom = roomId;
        userId = senderId; // 사용자 ID 저장
        rooms.get(roomId).add(socket);
      }

      // 같은 방의 사용자들에게 메시지 브로드캐스트
      rooms.get(roomId).forEach(client => {
        if (client !== socket && client.readyState === WebSocket.OPEN) {
          client.send(`[${roomId}] ${senderId}: ${msgContent}`);
        }
      });
    }
  });

  // 클라이언트 연결 종료 처리
  socket.on('close', () => {
    console.log(`${userId} disconnected from room ${currentRoom}`);
    if (currentRoom && rooms.has(currentRoom)) {
      rooms.get(currentRoom).delete(socket); // 방에서 클라이언트 제거
      if (rooms.get(currentRoom).size === 0) {
        rooms.delete(currentRoom); // 빈 방 삭제
      }
    }
  });
});

// 서버 상태 확인 로그
console.log('WebSocket server is running on ws://localhost:8080');
