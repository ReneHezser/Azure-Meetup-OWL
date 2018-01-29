var mqtt = require('mqtt')

var client  = mqtt.connect('mqtt://test.mosquitto.org',{
  will: {
    topic: 'AzureMeetupOwl/status',
    payload: 'offline',
    qos: 1,
    retain: true
  }
});
 
client.on('connect', function () {
  client.subscribe('AzureMeetupOwl')
  client.publish('AzureMeetupOwl/status', 'online')
})
 
client.on('message', function (topic, message) {
  // message is Buffer
  console.log(message.toString())
})
