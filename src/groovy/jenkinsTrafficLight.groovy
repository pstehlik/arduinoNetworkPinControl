@Grab(group='org.ccil.cowan.tagsoup', module='tagsoup', version='1.2' )

def jenkinsUrl = "http://192.168.27.19:8080/"
def arduinoIp = "192.168.26.231"
def pins = ['02','03','04'] //g,y,r - order is important

//red has precedence over yellow
def yellowWhenFailed = 1/10 //one out of ten failed (or more) -> traffic light turns yellow
def redWhenFailed = 2/10    //two out of ten failed (or more) -> traffic light turns red 


//set up the XML parsing
def tagsoupParser = new org.ccil.cowan.tagsoup.Parser()
def slurper = new XmlSlurper(tagsoupParser)
def htmlParser = slurper.parse(jenkinsUrl)

//counters for failed/succeeded tests
int failedTests = 0
int totalTests = 0
int disabledTests = 0

//parse the test results from the list view of jenkins
//tested on jenkins 1.426
htmlParser.'**'.findAll{ 
    it.@id == 'projectstatus'
}*.children()[0].children().findAll{
    it.name() == 'td'
}.findAll{
    it.children()[0].name() == 'img'
}*.children()*.each {
    if(it.@alt != "Disabled"){ totalTests++ } else { disabledTests++ }   

    if(it.@alt == "Failed"){ failedTests++ }
}

println "Enabled tests: [${totalTests}], disabled tests: [${disabledTests}], failed tests: [${failedTests}]"
if(totalTests <= 0){return}

def failureRatio = failedTests/totalTests
println "failureRatio: [${failureRatio}]"

// setting traffic lights
//default is all lights off
mainData = "3"
pins.each{
  mainData += "${it}0"
}

//determining which pin status should be replaced by 1 (turn on the pin)
def repl = null
if (redWhenFailed > 0 && failureRatio >= redWhenFailed){
  repl = 2  
} else if(yellowWhenFailed >0 && failureRatio >= yellowWhenFailed){
  repl = 1
} else {
 repl = 0
}

println "Turning pin [${pins[repl]}] on pos [${repl}] in config array on."

data = ''
if(repl != null){
  data = mainData.replace("${pins[repl]}0","${pins[repl]}1")
} else {
  data = mainData
}

//send the data for pin status to the listening arduino
println "sending " + data
data = data.getBytes()
addr = InetAddress.getByName(arduinoIp)
port = 10042
packet = new DatagramPacket(data, data.length, addr, port)
socket = new DatagramSocket()
socket.setSoTimeout(2000) // block for no more than 30 seconds
socket.send(packet)

buffer = (' ' * 4096) as byte[]
response = new DatagramPacket(buffer, buffer.length)
socket.receive(response)
s = new String(response.data, 0, response.length)
println "Server said: '$s'"