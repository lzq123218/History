package kwic.pf;

import java.io.CharArrayWriter;
import java.io.IOException;


public class LineTransformer extends Filter {

	public LineTransformer(Pipe input, Pipe output) {
		super(input, output);

	}

	protected void transform() {
	    try{
	        
	          // keeps the characters
	      CharArrayWriter writer = new CharArrayWriter();
	      
	      boolean newLine = true;
	      boolean collect = false;
	      
	      int c = input_.read();
	      while(c != -1){
	    	  
	    	  char val = (char) c;

	    	  // line has been read
	    	  if( newLine && (val >= 'A') && (val <= 'Z') ){
	    		  collect = true;
	    		  newLine = false;
	    	  }

	    	  if ( collect ){
	    		  
	    		  if ( val == ' '|| val == '\n'){
	    			  
	    			  String word = writer.toString();    			  
	    			  int len = word.length();	    			  
	    			  if ( len>0 ){
	    		
	    				  word = word.toUpperCase();
	    				  for (int i =0; i< len; i++){
	    					  
	    					  output_.write(word.charAt(i));
	    				  }
	    			  }	    			  
	    			  collect = false;
	    	    	  writer.reset();
	    	    	  
	    	    	  output_.write(c);
	    			  
	    		  }else{
	    			  
	    			  writer.write(c); 
	    		  }
	    		  
	    	  }else {
	    		  
	    		  output_.write(c);
	    	  }
	    	  
	    	  if (val == '\n'){
	    		  newLine = true;
	    	  }

	    	  c = input_.read();
	      }

	          // close the pipe
	      output_.closeWriter();
	    }catch(IOException exc){
	      exc.printStackTrace();
	      System.err.println("KWIC Error: Could not make circular shifts.");
	      System.exit(1);
	    }		
	}

}
