addr = InetAddress.getByName("192.168.8.2")
port = 10042
socket = new DatagramSocket()

commands = [
" 30",
" 40",
" 50",
" 60"
]
commands.each{ singleCommand ->
  data = singleCommand.getBytes()
  packet = new DatagramPacket(data, data.length, addr, port)
  socket.send(packet)
}