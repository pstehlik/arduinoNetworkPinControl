//setting 3 pins and their states in a changing pattern

//default state is pins 2,3,4 as off (0)
mainData = "3020030040"

//IP address of the arduino
arduinoIp = "192.168.26.231"

//loop through the pins up and down and up and turn each light on, while the others remain off
['02','03','04','03','02','03','04'].each{ 
    data = mainData.replace("${it}0","${it}1")
    println "sending " + data
    data = data.getBytes()
    addr = InetAddress.getByName(arduinoIp)
    port = 10042
    packet = new DatagramPacket(data, data.length, addr, port)
    socket = new DatagramSocket()
    socket.setSoTimeout(3000) // block for no more than 30 seconds
    socket.send(packet)

    buffer = (' ' * 4096) as byte[]
    response = new DatagramPacket(buffer, buffer.length)
    socket.receive(response)
    s = new String(response.data, 0, response.length)
    println "Server said: '$s'"
}

