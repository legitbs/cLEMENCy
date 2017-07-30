class Chapter
  CHAPTER_NAME_MATCHER = /^-([^-].+)-$/

  def initialize()
    @lines = []
  end
  
  def consume(data)
    parse_chapter_name(data)

    loop do
      begin
        return @more = true if new_chapter_starting?(data)
      rescue StopIteration
        return @more = false
      end

      
      @lines << data.next
    end
  end

  def to_tex
    [
      "\\chapter{#{@name}}",
      @lines
    ].flatten.join("\n")
  end

  def more?
    @more
  end
  
  private
  def new_chapter_starting?(data)
    return true if data.peek =~ CHAPTER_NAME_MATCHER
  end
  
  def parse_chapter_name(data)
    header = data.next
    md = CHAPTER_NAME_MATCHER.match header
    raise "couldn't find chapter name in #{header.inspect}" if md.nil?
    @name = md[1]
  end
  
  class Subchapter
    SUBCHAPTER_NAME_MATCHER = /^--(.+)--$/
  end
end

if __FILE__ == $0
  data = File.open('chapters.txt', 'r').each_line.lazy.map(&:strip)
  found = []
  loop do
    cur = Chapter.new
    more = cur.consume(data)
    found << cur
    break unless cur.more?
  end

  p found
end
