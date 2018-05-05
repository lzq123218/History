package kwic.es;

import java.util.Observable;
import java.util.Observer;
import java.util.HashMap;
import java.lang.Integer;
import java.util.function.BiConsumer;

class MapConsumer implements BiConsumer<String, Integer>{

	public void accept(String key, Integer val) {
		
		System.out.print(key);
		System.out.print(" : ");
		System.out.println(val);
	}	
}



public class WordsIndex implements Observer {

	private HashMap<String,Integer> hm_ = new HashMap<String,Integer>();
	//<String, int> has error,why
		
	private void addIndex(String str){
		Integer val = 1;
		
		if ( hm_.containsKey(str)){
			
			val = hm_.get(str);
			val ++;			
			hm_.replace(str, val);
						
		}else{
			
			hm_.put(str, val);
		}		
	}
	
	private void decIndex(String str){
		
		if ( hm_.containsKey(str)){
			
			Integer val = hm_.get(str);
			val --;
			if ( val > 0 ){				
				hm_.replace(str, val);
				
			}else{
				hm_.remove(str);				
			}			
		}		
	}
	
	public void outPut(){
		
		if (!hm_.isEmpty()){
			
			MapConsumer action = new MapConsumer();
			hm_.forEach(action);
		}		
	}

	public void update(Observable observable, Object arg) {

		LineStorageWrapper lines = (LineStorageWrapper) observable;
		LineStorageChangeEvent event = (LineStorageChangeEvent) arg;

		switch(event.getType()){

		case LineStorageChangeEvent.ADD:

			// get the last added line
			String[] line = lines.getLine(lines.getLineCount() - 1);

			for(int i = 0; i < line.length; i++){

				addIndex(line[i]);
			}
			break;
		case LineStorageChangeEvent.DELETE:

			String[] line_deleted = lines.getDeletedLine();    	
			for(int i = 0; i < line_deleted.length; i++){

				decIndex(line_deleted[i]);
			}    	
			break;
		default:
			break;      
		}		
	}
}
