//setting 4 pins and their states
//3 - 1, 4 - 1, 5 - 0, 6 - 1 
data = "4031041050061Some Message String With Long Content".getBytes()
addr = InetAddress.getByName("192.168.8.2")
port = 10042
packet = new DatagramPacket(data, data.length, addr, port)
socket = new DatagramSocket()
socket.setSoTimeout(4000) // block for no more than 30 seconds
socket.send(packet)


buffer = (' ' * 4096) as byte[]
response = new DatagramPacket(buffer, buffer.length)
socket.receive(response)
s = new String(response.data, 0, response.length)
println "Server said: '$s'"