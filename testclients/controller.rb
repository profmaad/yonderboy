#!/usr/bin/env ruby

require 'readline'
require 'eventmachine'
require 'pp'

CONTROLLER_SOCKET = "/home/profmaad/.cli-browser/controller.sock"

@controllerSocket = ARGV[0]

class CLIBrowserClient < EventMachine::Connection
  def initialize(*args)
    super
  end
  def post_init
    @packetID = 0
    @receiveBuffer = ""

    package = construct_packet({"type" => "connection-management", "id" => "init00", "command" => "initialize"})

    send_data package
  end

  def receive_data(data)
    @receiveBuffer += data

    if(@receiveBuffer =~ /\n\n$/)
      handle_packet(parse_packet(@receiveBuffer.chomp))
      @receiveBuffer = ""
    end
  end

  def parse_packet(raw)
    packet = Hash.new
    raw.each_line do |line|
      if(line =~ /^(.*)=(.*)\n$/)
        key = $1.strip
        value = $2.strip
        packet[key] = value
      else
        packet[line] = ""
      end
    end
    
    return packet
  end
  def construct_packet(values)
    data = ""
    values.each do |key, value|
      data += "#{key} = #{value}\n"
    end
    if(!values["id"])
      data += "id = #{@packetID}\n"
    end
    @packetID += 1
    data += "\n"

    return data
  end

  def handle_packet(packet)
    print "Received: "
    pp packet
  end

  def unbind
    puts "Connection closed"
    EventMachine::stop_event_loop
  end
end

module KeyboardHandler
  include EventMachine::Protocols::LineText2
  def receive_line(line)
    handle_line(line)
  end
  def handle_line(line)
    if !@values
      @values = Hash.new
      @values["type"] = "command"
    end

    if line == "."
      print "Send: "
      pp @values
      $ipcSocketSignature.send_data($ipcSocketSignature.construct_packet(@values))
      @values = Hash.new
      @values["type"] = "command"
    elsif line =~ /^(.*)=(.*)$/
      @values[$1.strip] = $2.strip
    else
      @values[line] = ""
    end
  end
end


EventMachine::run do
  EventMachine::open_keyboard(KeyboardHandler)

  $ipcSocketSignature = EventMachine::connect_unix_domain @controllerSocket ? @controllerSocket : CONTROLLER_SOCKET, CLIBrowserClient
end
