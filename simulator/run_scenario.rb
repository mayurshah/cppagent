require 'time'
require 'socket'
require 'optparse'

loop_file = false
port = 7878
scenario = false
verbose = false

OptionParser.new do |opts|
  opts.banner = 'Usage: run_scenrio.rb [-l] <file>'

  opts.on('-l', '--[no-]loop', 'Loop file') do  |v|
    loop_file = v
  end

  opts.on('-p', '--port [port]', OptionParser::DecimalNumeric, 'Port (default: 7878)') do  |v|
    port = v
  end

  opts.on('-h', '--help', 'Show help') do
    puts opts
    exit 1
  end

  opts.on('-s', '--[no-]scenario', 'Run scenario or log') do |v|
    scenario = v
  end
  
  opts.on('-v', '--[no-]verbose', 'Verbose output') do |v|
    verbose = v
  end

  opts.parse!
  if ARGV.length < 1
    puts "Missing log file <file>"
    puts opts
    exit 1
  end
end

puts "Waiting on 0.0.0.0 #{port}"
server = TCPServer.new(port)
socket = server.accept
puts "Client connected"
def format_time
  time = Time.now.utc
  time.strftime("%Y-%m-%dT%H:%M:%S.") + ("%06d" % time.usec)
end

if scenario
  begin
    File.open(ARGV[0]) do |file|
      file.each do |l|
        f, r = l.chomp.split(' ')
        r = 1 unless r
        
        line = "#{format_time}|#{f}"
        socket.puts line
        puts line if verbose
        sleep(r.to_i)
      end
    end
  end while (loop_file)
else
  begin
    File.open(ARGV[0]) do |file|
      last = nil
      file.each do |l|
        next unless l =~ /^\d{4,4}-\d{2,2}-\d{2,2}.+\|/o
        f, r = l.chomp.split('|', 2)
        time = Time.parse(f)
        
        # Recreate the delta time
        if last
          delta = time - last
          sleep delta if delta > 0.0
        end
        
        line = "#{format_time}|#{r}"
        socket.puts line
        puts  line if verbose
        
        if select([socket], nil, nil, 0.00001)
          begin
            if (r = socket.read_nonblock(256)) =~ /\* PING/
              puts "Recived #{r.strip}, responding with pong"
              socket.puts "* PONG 10000"
              socket.flush
            end
          rescue
          end
        end
        last = time
      end
    end
  end while (loop_file)
end
