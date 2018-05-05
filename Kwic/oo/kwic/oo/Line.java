package kwic.oo;

import java.util.ArrayList;

public class Line {
	
	private ArrayList<String> words_ = new ArrayList<String>();
	
	public void add(String word){
		
		words_.add( word );
	}
	
	public int size(){
		
		return words_.size();
	}
	
	public void clear(){
		
		words_.clear();
	}
	
	public void remove( int index ){
	
		words_.remove(index);
	}
	
	public String get( int index ){
	
		return words_.get(index);
	}
	
	public void set(int index, String element){
		
		words_.set(index, element);
	}
	


}
