require 'pry'
require 'set'
require 'stringio'
require 'digest'
require 'to_latex'
require 'nokogiri'
require 'fileutils'
require 'titlecase'

INSTRUCTION_BUFF_OUT_LINES = 35

module ChildElements
  def child_elements
    children.select{ |n| n.is_a? Nokogiri::XML::Element }
  end
end

class Nokogiri::XML::NodeSet
  include ChildElements
end
class Nokogiri::XML::Element
  include ChildElements
end

module MysteryElement
  def mystery_element(child)
    <<-EOT
\\textit{unknown #{child.name}}
{ \\scriptsize
\\begin{verbatim}
#{child.pretty_inspect}
\\end{verbatim}
}
EOT
  end
end

class Chapters
  def initialize
    @in = Nokogiri::HTML(File.read('chapters.html5'))
    @out = File.open('chapters.tex', 'w')
  end

  def process
    @in.css('body').children.each do |child|
      case child
      when Nokogiri::XML::Text
        @out.puts child.text.to_latex
      when Nokogiri::XML::Element
        process_element child
      else
        @out.puts "\\textit{unknown #{child.class.name}}"
      end
    end
  end

  def finish
    @out.close
  end

  private
  def process_element(child)
    case child.name
    when 'h1'
      @out.puts "\\chapter{#{child.text}}"
    when 'h2'
      @out.puts "\\section{#{child.text}}"
    when 'br'
      @out.puts
    when 'table'
      @out.puts Table.new(child).to_latex
    else
      @out.puts "\\textit{unknown #{child.name}}:"
      @out.puts "\\scriptsize"
      @out.puts "\\begin{verbatim}"
      @out.puts(child.pretty_inspect)
      @out.puts "\\end{verbatim}"
      @out.puts "\\normalsize"
    end
  end
end

class Instructions
  include MysteryElement
  def initialize
    @in = Nokogiri::HTML(File.read('instructions.html5'))
    @out = File.open('instructions.tex', 'w')
    @hints = Set.new(File.read('hints.txt').lines.map(&:strip))
  end

  def process
    @buf = StringIO.new
    @in.css('body').child_elements.each do |child|
      case child.name
      when 'h2'
        buff_out
        mnemonic, name = child.child_elements.map(&:text)
        section_name = "#{mnemonic.upcase}: #{name.titlecase}"
        if @hints.include? mnemonic.upcase
          @buf.puts "\\pagebreak"
        end
        @buf.puts "\\section{#{section_name}}"
        @buf.puts "\\index{#{mnemonic.upcase}}"
      when 'table'
        unless 'fields' == child.attr('class')
          next mystery_element child
        end

        FieldTable.new(child).process(@buf)
      when 'dl'
        process_dl(child)
      else
        @buf.puts mystery_element(child)
      end
    end

    buff_out
  end

  def buff_out
    if @buf.string.lines.select{  |l| ! l.empty? }.count > INSTRUCTION_BUFF_OUT_LINES
      @out.puts "%% LONG ONE"
      @out.puts "\\pagebreak"
      @out.puts @buf.string
      @out.puts "\\vfill"
      @out.puts "\\pagebreak"
    else
      @out.puts "\\pagebreak[3]"
      @out.puts @buf.string
      @out.puts "\\vfill"
    end

    @buf = StringIO.new
  end

  def finish
    @out.close
  end

  private

  def process_dl(el)
    @buf.puts "\\begin{description}"
    el.child_elements.each do |child|
      if 'dt' == child.name
        @buf.write "\\item [#{child.text}] "
        next
      end

      case child.attr('class')
      when 'format'
        @buf.puts "\\texttt{#{child.text.to_latex}}"
      when 'flags'
        if child.text.empty?
          @buf.puts "\\textit{None}"
        else
          @buf.puts "\\texttt{#{child.text.to_latex}}"
        end
      when 'description'
        Description.new(child).process(@buf)
      when 'operation'
        Operation.new(child).process(@buf)
      else
        @buf.puts child.text.to_latex
      end
    end
    @buf.puts "\\end{description}"
  end
end

class Description
  include MysteryElement

  def initialize(el)
    @el = el
  end

  def process(out)
    @el.children.each do |child|
      if child.is_a? Nokogiri::XML::Text
        out.puts child.text unless child.text.empty?
        out.puts
        next
      end

      case child.name
      when 'br'
        out.puts "\\nopagebreak"
      when 'table'
        table = Table.new(child)
        out.puts "\\nopagebreak[4]"
        out.puts table.to_latex
        out.write "%% table row\n" * (table.row_count - 1)
      else
        out.puts mystery_element(child)
      end
    end
  end
end

class Operation
  include MysteryElement
  def initialize(el)
    @el = el
  end

  def process(out)
    out.puts "\\begin{verbatim}"
    @el.children.each do |child|
      if child.is_a? Nokogiri::XML::Text
        out.write child.text.
                    gsub("\xc2\xa0\xc2\xa0", ' ')
                    # gsub('_', "\\\\_").
                    # gsub("\u2190", "\\leftarrow").
                    # gsub('&', " \\\\&").
                    # gsub('%', "\\\\%")
        next
      end

      case child.name
      when 'br'
        ''
      else
        out.puts mystery_element(child)
      end
    end
    out.puts "\\end{verbatim}"
  end

  def single_text(out)
    words = @el.children.first.text.split.map do |w|
      w.
        gsub('_', "\\\\_").
        gsub(/\w+/){|w| "\\mathtt{#{w}}"}.
        gsub("\u2190", "\\leftarrow").
        gsub('&', " \\\\&").
        gsub('%', "\\\\%")
    end

    out.puts "\\(#{words.join ' '}\\)"
  end
end

class FieldTable
  def initialize(el)
    @el = el
    @rows = @el.css('tr')
  end

  def process(out)
    out.puts "{"
    out.puts "\\setlength{\\tabcolsep}{3pt}"
    out.puts "\\begin{tabu} spread \\linewidth {#{column_desc}}"

    out.puts header_data
    out.puts detail_data
    out.puts "\\end{tabu}"
    out.puts "}"
    out.puts "\\nopagebreak"
  end

  private

  def column_desc
    header_row.child_elements.map do |el|
      align_class(el)
    end.join(' ')
  end

  def header_data
    header_row.child_elements.map.with_index do |el, i|
      t = el.text
      left_pipe = '|'
      if (0 == i) || ('multibit-end' == el.attr('class'))
        left_pipe = ''
      end

      "\\multicolumn{1}{#{ left_pipe }#{ align_class el }}{#{t}}"
    end.join(" & ") + " \\\\"
  end

  def align_class(el)
    case el.attr 'class'
      when 'multibit-start'
        "l"
      when 'multibit-end'
        "r"
      when 'singlebit'
        "c"
    end
  end

  def detail_data
    detail_row.child_elements.map.with_index do |el, i|
      colspan = (el.attr('colspan') || '1').to_i
      left_pipe = '|'
      if 0 == i
        left_pipe = ''
      end

      t = el.text

      if t =~ /^[01]+$/
        t = "\\texttt{#{t}}"
      end

      "\\multicolumn{#{colspan}}{#{left_pipe}c}{#{t}}"
    end.join(' & ')
  end

  def header_row
    @rows[0]
  end

  def detail_row
    @rows[1]
  end
end

class Table
  def initialize(el)
    @element = el
    @checksum = Digest::SHA256.hexdigest(@element.to_html)
  end

  def to_latex
    build_file unless latex_file?

    return file_input
  end

  def row_count
    @element.css('tr').count
  end

  private
  def build_file
    File.open(latex_filename, 'w') do |out|
      out.puts "\\texttt{#{@checksum}}"

      rows = @element.css('tr')
      col_count = rows.first.css('td').count
      row_count = rows.count
[]
      column_desc = ' X ' * col_count

      head, *tail = rows

      out.puts "\\begin{tabu} to \\linewidth {#{column_desc}}"

      process_tr(out, head)

      out.puts "\\hline"


      tail.each{ |row| process_tr(out, row) }

      out.puts "\\end{tabu}"
    end
  end

  def process_tr(out, row)
    out.write(row.css('td').map do |cell|
                 cell.text.to_latex
               end.
                 join(" & "))
    out.puts " \\\\"
  end

  def latex_file?
    File.exist? latex_filename
  end

  def file_input
    FileUtils.touch(usage_filename)
    "\\input{#{latex_filename}}"
  end

  def latex_filename
    "./tables/#{@checksum}.tex"
  end

  def usage_filename
    "./tables/#{@checksum}.last_used"
  end
end

FileUtils.mkdir_p("./tables")

ch = Chapters.new
ch.process
ch.finish

is = Instructions.new
is.process
is.finish
