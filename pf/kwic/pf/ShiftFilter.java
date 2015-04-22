package kwic.pf;

import java.io.BufferedReader;
import java.io.CharArrayWriter;
import java.io.FileNotFoundException;
import java.io.FileReader;
import java.io.IOException;
import java.util.StringTokenizer;
import java.util.HashSet;



public class ShiftFilter extends Filter {
	
	public ShiftFilter(Pipe input, Pipe output) {
		super(input, output);
	}

	protected void transform() {

	    try{

	      CharArrayWriter writer = new CharArrayWriter();
	      
	      int c = input_.read();
	      while(c != -1){
	        
	        if(((char) c) == '\n'){
	          
	          String line = writer.toString();
	          
	          StringTokenizer tokenizer = new StringTokenizer(line);
	          boolean discard = false;
	          
	          if (tokenizer.countTokens() > 0){
	        	  
	        	  String first = tokenizer.nextToken();	
	        	  discard = find(first);
	          }

	          if ( !discard ){
	        	  
		          line = line + "\n";
		
		          char[] chars = line.toCharArray();
		          for(int j = 0; j < chars.length; j++)
		        	  output_.write(chars[j]);
	          }
	          
	              // reset the character buffer
	          writer.reset();
	        }else
	          writer.write(c);
	        
	        c = input_.read();
	      }

	      output_.closeWriter();
	    }catch(IOException exc){
	      exc.printStackTrace();
	      System.err.println("KWIC Error: Could not make circular shifts.");
	      System.exit(1);
	    }	
	}

	public void setFilter(String file){
		if ( file == null) return;
		try{

			BufferedReader reader = new BufferedReader(new FileReader(file));

			// read all lines until EOF occurs
			// (Note that all line feed chracters are removed by the readLine() method)
			String line = reader.readLine();
			while(line != null){

				items_.add(line);
				line = reader.readLine();
			}

		}catch(FileNotFoundException exc){

			// handle the exception if the file could not be found
			exc.printStackTrace();
			System.err.println("KWIC Error: Could not open " + file + "file.");
			System.exit(1);

		}catch(IOException exc){

			// handle other system I/O exception
			exc.printStackTrace();
			System.err.println("KWIC Error: Could not read " + file + "file.");
			System.exit(1);

		}
	}
		
	private boolean find( String word){
		return items_.contains(word);
	}
	
	private HashSet<String> items_ = new HashSet<String>();
}
