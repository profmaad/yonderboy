#!/usr/bin/ruby

require 'socket'
require 'gtk2'
require 'pp'

#$stdout.reopen("/tmp/viewer_rb.log","w")
#$stdout.sync = true
#$stderr.reopen($stdout)
#$stderr.sync = true

def parsePacket
  pp @buffer

  if(@buffer["type"] == "command")
    if(@buffer["command"] == "connect-to-renderer")
      @gtkSocket.add_id(@buffer["display-information"].to_i)
    elsif(@buffer["command"] == "disconnect-from-renderer")
      @gtkSocket.add_id(0)
    end
  elsif(@buffer["type"] == "ack" && @buffer["id"] == "0")
    @socket.write("type = connection-management\ncommand = register-view\nview-id = view00\ndisplay-information = #{@gtkSocket.id}\ndisplay-information-type = XEMBED\nid = 1\n\n")
  end
    
  if(@buffer["id"])
    @socket.write("type = ack\nid = #{@buffer["id"]}\n\n")
  end

  @buffer = Hash.new
end

# GUI elements
@gtkSocket = Gtk::Socket.new
@window = Gtk::Window.new

@window.signal_connect("destroy"){Gtk.main_quit}
@gtkSocket.signal_connect("plug-added") do 
  puts "plug was added, new window: #{@gtkSocket.plug_window}"
  @window.show_all
end

@window.add(@gtkSocket)
@window.show_all
# TEMP END

# IPC socket stuff
@socketFile = ARGV[0]
@socket = UNIXSocket.open(@socketFile)

# read buffer
@buffer = Hash.new

@socket.write("type = connection-management\ncommand = initialize\nid = 0\nclient-name = ruby-test\ncan-display-stati\n\n")

@thread = Thread.new {
  Kernel.loop do
    if(@socket.eof?)
      exit(0)
    end
    line = @socket.readline
    if(line == "\n")
      parsePacket()
    elsif(line =~ /^(.*)=(.*)\n$/)
      key = $1.strip
      value = $2.strip
      @buffer[key] = value
    end
  end
}

Gtk.main

@socket.close

@thread.join
