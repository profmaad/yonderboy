#!/usr/bin/ruby

require 'socket'
require 'gtk2'
require 'webkit'

$stdout.reopen("/tmp/renderer_rb.log","w")
$stdout.sync = true
$stderr.reopen($stdout)
$stderr.sync = true

def parsePacket
  if(@buffer["type"] == "command")
    if(@buffer["command"] == "open-uri")
      @webview.open(@buffer["uri"])
    elsif(@buffer["command"] == "reload-page")
      @webview.reload
    end
  end
    
  if(@buffer["id"])
    @socket.write("type = ack\nack-id = #{@buffer["id"]}\n\n")
  end

  @buffer = Hash.new
end

# GUI elements
@webview = Gtk::WebKit::WebView.new
@plug = Gtk::Plug.new

# IPC socket stuff
@socketFD = ARGV[0].to_i
@socket = nil

# read buffer
@buffer = Hash.new

puts "My socket fd is #{@socketFD}"

@socket = IO.new(@socketFD, "a+")

@socket.write("type = connection-management\ncommand = initialize\nid = init00\nclient-name = ruby-webkit-test\nclient-version = 0\nbackend-name = webkit\nbackend-version = r51110\ndisplay-information-type = XEMBED\ndisplay-information = #{@plug.id}\n\n")

Kernel.loop do
  if(@socket.eof?)
    exit(0)
  end
  line = @socket.readline
  if(line == "\n")
    parsePacket()
  elsif(line =~ /^(.*) *= *(.*)\n$/)
    @buffer[$1] = $2
  end
end
