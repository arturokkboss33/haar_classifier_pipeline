///standard c++ libs
#include <string.h>
#include <math.h>
#include <iostream>
#include <fstream>

///boost libs
#include "boost/filesystem.hpp"

///opencv libs
#include "opencv2/core/core.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"

///global variables
//boost
namespace boost_fs = boost::filesystem;
//input params
std::string params_dir = "param_files/crop_samples.yml";
std::string processedData_dir;
std::string patches_basename;
bool flag_continue_crop = false;
bool flag_crop_bg = false;
int min_patch_size = 20;
//output params
std::string file_ref_name;
std::string file_ref_basename= "file_ref.yml";
std::string file_ref_tmp_name;
std::string file_ref_tmp_basename= "file_ref_tmp.yml";
std::string file_inputs_basename = "inputs.dat";
std::string file_inputs_name;
std::ofstream file_inputs_stream;
std::string processedData_dir_name;
std::string patches_dir_name;
std::string masks_dir_name;
std::string bg_dir_name;
std::string parent_dir_name;
cv::FileStorage fstore_file_ref;
cv::FileStorage fstore_file_ref_tmp;
cv::FileStorage* ptr_fstore;
//opencv images
cv::Mat img;
cv::Mat patch_mask;
cv::Point corner1,corner2;
cv::Rect box;
//state machine mouse-click variables
int state_crop = 0;
bool next_image = false;
bool leftUp = false;
bool leftDown = false;
bool display_info = true;
bool flag_wait_input = true;
int continue_counter = 0;
int exit_counter = 0;
//other variables
std::string underscore = "_";
std::string slash = "/";
std::string file_extension = ".jpg";
std::string str_patches = "patches";
std::string str_masks = "masks";
std::string str_bg = "bg";
int patch_count = 0;
int bg_count = 0;
bool flag_user_stop = false;
bool flag_old_file = false;
std::string dir_tree_basename= "dir_tree_full.dat";
std::string dir_tree_name;
std::ofstream dir_tree_streamW;
std::ifstream file_inputs_streamR;

void menu_continueCrop(){
	std::cout << std::endl;
	std::cout << "************" << std::endl;
	std::cout << "Press C to continue" << std::endl;
	std::cout << "Press Q to quit or take a break" << std::endl;
	std::cout << "************" << std::endl;

}

///@FCN mouse callback
void mouse_call(int event,int x,int y,int,void*){
	
	switch(state_crop){
		
		///state where no patch has been picked
		case 0:
			if(event==cv::EVENT_LBUTTONDOWN){
				leftDown=true; corner1.x=x;corner1.y=y;
				std::cout << "\n *** SELECTING PATCH ***" << std::endl;
				std::cout <<"Corner 1: "<< corner1 << std::endl;
			}else if(event==cv::EVENT_LBUTTONUP){
				
				if(abs(x-corner1.x)>min_patch_size && abs(y-corner1.y)>min_patch_size){ //checking whether the region is too small
					leftUp=true; corner2.x=x; corner2.y=y;
					std::cout << "Corner 2: " << corner2 << std::endl;
				}else{
					std::cout << "Select a region more than " << min_patch_size <<" pixels per dim" << std::endl;
				}
			}else if(leftDown==true && leftUp==false){ //when the left button is down
				cv::Point pt; pt.x=x; pt.y=y;
				cv::Mat temp_img = img.clone();
				cv::rectangle(temp_img,corner1,pt,cv::Scalar(0,0,255)); //drawing a rectangle continuously
				cv::imshow("Original",temp_img);
			}else if(leftDown==true&&leftUp==true){ //when the selection is done
				leftDown=false;
				leftUp=false;
				std::cout << "\n ??? SAVE THE PATCH ???" << std::endl;
				std::cout << "RIGHT CLICK: YES" << std::endl;
				std::cout << "LEFT CLICK: NO" << std::endl;
				state_crop = 1;
			}
			break; ///case 0
			
			///check whether to save the patch or not
			case 1:
			
				if(event == cv::EVENT_RBUTTONDOWN){
					std::cout << "\n IMAGE SAVED" << std::endl;
					box.width=std::abs(corner1.x-corner2.x);
					box.height=std::abs(corner1.y-corner2.y);
					box.x=std::min(corner1.x,corner2.x);
					box.y=std::min(corner1.y,corner2.y);
					cv::Mat crop(img,box);   //Selecting a ROI(region of interest) from the original pic
					
					//cv::Mat mask = cv::Mat::zeros(box.height,box.width, CV_8UC1);
					//mask.copyTo(patch_mask(box));

					std::ostringstream convert;
					if(!flag_crop_bg){
						cv::Mat mask = cv::Mat::zeros(box.height,box.width, CV_8UC1);
						mask.copyTo(patch_mask(box));
						
						convert << patch_count;
						std::string no_image = convert.str();
						std::string image_name = patches_dir_name + slash + patches_basename + 
							underscore + no_image + file_extension;
						
						cv::imwrite(image_name, crop);
						convert.clear();
						patch_count++;
						
						(*ptr_fstore) << "{";
						(*ptr_fstore) << "patch_name" << image_name;
						(*ptr_fstore) << "center_x" << int(round(box.x+box.width/2));
						(*ptr_fstore) << "center_y" << int(round(box.y+box.height/2));
						(*ptr_fstore) << "height" << box.width;
						(*ptr_fstore)<< "width" << box.height; 
						(*ptr_fstore) << "}";
					}else{
						convert << bg_count;
						std::string no_bg = convert.str();
						std::string bg_image_name = bg_dir_name + slash + str_bg + 
							underscore + no_bg + file_extension;
						bg_count++;
						cv::imwrite(bg_image_name,crop);
					}
					
					state_crop = 2;
					leftDown=false;
					leftUp=false;
					
				}else if(event==cv::EVENT_LBUTTONUP){
					std::cout << "\n PATCH NOT SELECTED" << std::endl;
					
					state_crop = 2;
					leftDown=false;
					leftUp=false;
				}
			
			break;	///end case 1
			
			///check whether to continue cropping the same image or go to next one
			case 2:
					if(display_info){
						std::cout << "\n ??? NEXT ACTION ???" << std::endl;
						std::cout << "DOUBLE LEFT CLICK: GO TO THE NEXT IMAGE" << std::endl;
						std::cout << "DOUBLE RIGHT CLICK: CONTINUE CROPPING THIS IMAGE" << std::endl;
						display_info = false;
					}

					if(event==cv::EVENT_LBUTTONUP){ 
						exit_counter++;
						continue_counter = 0;
						if(exit_counter == 2){
							std::cout << "\n GOING TO THE NEXT IMAGE" << std::endl;
							exit_counter = 0;
							display_info = true;
							next_image = true;
							state_crop = 0;		
							 				
						}
						
					}else if(event==cv::EVENT_RBUTTONUP){
						continue_counter++;
						exit_counter = 0;
						if(continue_counter == 2){
							std::cout << "\n CONTINUE CROPPING" << std::endl;
							continue_counter = 0;
							display_info = true;
							state_crop = 0;
						}
					}
					
			break; ///end case 2
		
	}
}

///@FCN crop image patches
void crop_image(){
	
	cv::namedWindow("Original");
	cv::imshow("Original",img);

	cv::setMouseCallback("Original",mouse_call); //setting the mouse callback for selecting the region with mouse

	while(char(cv::waitKey(1)!='p') && !next_image){ //waiting to finish going through all the files
	}
	next_image = false;
}

void wait_user(){
	cv::namedWindow("Original");
	cv::imshow("Original",img);
	menu_continueCrop();
	
	bool flag_wait_user = true;
	
	while(flag_wait_user){
		char keycode = cv::waitKey(0);
		//std::cout << keycode << std::endl;
		if(keycode == 'c'){
			std::cout << std::endl;
			std::cout << "CONTINUE" << std::endl;
			flag_wait_user = false;
		}else if(keycode == 'q'){
			std::cout << std::endl;
			std::cout << "!!! EXITING PROGRAM !!!" << std::endl;
			flag_user_stop = true;
			flag_wait_user = false;
		}
	}
}

bool isFileProcessed(std::string file){
	
	std::string line;
	file_inputs_streamR.open(file_inputs_name.c_str(),std::ifstream::in);
	while(getline(file_inputs_streamR,line)){
		//std::cout << line << std::endl;
		if(line.compare(file)==0){
			file_inputs_streamR.close();
			return true;
		}
	}
	
	file_inputs_streamR.close();
	return false;
}

///@FCN traverse a directory recursively
void iterate_dir(const boost_fs::path& basepath, std::string parent_dir_name){
	
	for(boost_fs::directory_iterator iter(basepath), end; iter != end && !flag_user_stop; ++iter){
		boost_fs::directory_entry entry = *iter;
		
		if(boost_fs::is_directory(entry.path())){
			std::cout << "Processing directory: " << entry.path().string() << std::endl;
			
			std::string new_dir_name =  parent_dir_name + slash + entry.path().leaf().c_str();
			boost_fs::path path_new_dir(new_dir_name);
			if(!boost_fs::exists(path_new_dir)){
				if(boost_fs::create_directory( boost_fs::path(new_dir_name) )){
					std::cout << "*** Masks sub-directory created sucessfully" << std::endl;
					std::cout << new_dir_name << std::endl;
					std::cout << std::endl;
				}
			}else{
				std::cout << "*** Existing masks sub-directory" << std::endl;
				std::cout << new_dir_name << std::endl;
				std::cout << std::endl;
			}			
			///Iterate recursively
			iterate_dir(entry.path(), new_dir_name);
		}
		else{
			
			boost_fs::path entryPath = entry.path();
			if(entryPath.extension()==".jpg" || entryPath.extension()==".bmp" || entryPath.extension()==".png" ){
				
				///Output file name and save it in a reference file
				std::cout << "\n Processing file: " << entry.path().string() << std::endl;	
				
				///If the program was used before; checked if the file was already processed
				if(flag_continue_crop){
					flag_old_file = isFileProcessed(entry.path().string());
				}else{
					flag_old_file = false;
				}
				if(!flag_old_file){
					///Include the file in the directory of processed files
					file_inputs_stream.open(file_inputs_name.c_str(),std::ofstream::app);
					file_inputs_stream << entry.path().string() << "\n";
					file_inputs_stream.close();	
							
					///To crop image
					img = cv::imread(entryPath.string(),CV_LOAD_IMAGE_COLOR);
					if(!img.empty()){
						
						std::string mask_image_name = parent_dir_name + slash + str_masks + 
							underscore + entryPath.leaf().c_str();
														
						///Update the file ref						
						(*ptr_fstore) << "{";
						(*ptr_fstore) << "input_image" << entry.path().string();
						(*ptr_fstore) << "patches_mask" <<  mask_image_name;
						(*ptr_fstore) << "patches" << "[";
						
						///Call cropping method
						patch_mask = cv::Mat::ones(img.rows,img.cols,CV_8UC1)*255;
						crop_image();
						(*ptr_fstore) << "]";
						(*ptr_fstore) << "}";
						
						///Create bg image, save bg and mask image
						cv::Mat tmp_mask;
						cv::cvtColor(patch_mask, tmp_mask, CV_GRAY2BGR, 3);
						cv::Mat bg_img = img & tmp_mask;
						std::ostringstream convert;
						convert << bg_count;
						std::string no_bg = convert.str();
						std::string bg_image_name = bg_dir_name + slash + str_bg + 
							underscore + no_bg + file_extension;
						bg_count++;
						cv::imwrite(bg_image_name,bg_img);
						cv::imwrite(mask_image_name, patch_mask);
						
						///Ask the user whether to continue the program or not
						wait_user();				
						

					}else{
						std::cout << "Fail to load the image" << std::endl;
					}
				
				}else{
					std::cout << "File previously processed" << std::endl;
				}
				
				
			}
		}
	}

	
}

///@FCN traverse a directory recursively
void iterate_dir_bg(const boost_fs::path& basepath){
	
	for(boost_fs::directory_iterator iter(basepath), end; iter != end && !flag_user_stop; ++iter){
		boost_fs::directory_entry entry = *iter;
		
		if(boost_fs::is_directory(entry.path())){
			std::cout << "Processing directory: " << entry.path().string() << std::endl;
			///Iterate recursively
			iterate_dir_bg(entry.path());
		}
		else{
			
			boost_fs::path entryPath = entry.path();
			if(entryPath.extension()==".jpg" || entryPath.extension()==".bmp" || entryPath.extension()==".png" ){
				
				///Output file name and save it in a reference file
				std::cout << "\n Processing file: " << entry.path().string() << std::endl;	
						
				///To crop image
				img = cv::imread(entryPath.string(),CV_LOAD_IMAGE_COLOR);
				if(!img.empty()){
					
					///Call cropping method
					crop_image();
					
					///Ask the user whether to continue the program or not
					wait_user();				

				}else{
					std::cout << "Fail to load the image" << std::endl;
				}
				
			}
		}
	}

	
}

int main ( int argc, char *argv[] ){
	
	///Read input parameters
	std::string source_dir;
	cv::FileStorage fstore_params_dir(params_dir, cv::FileStorage::READ);
	fstore_params_dir["source_dir"] >> source_dir;
	fstore_params_dir["processedData_dir"] >> processedData_dir;
	fstore_params_dir["patches_basename"] >> patches_basename;
	fstore_params_dir["continue_crop"] >> flag_continue_crop;
	fstore_params_dir["min_patch_size"] >> min_patch_size;
	fstore_params_dir["crop_bg"] >> flag_crop_bg;
	fstore_params_dir.release();
	
	std::cout << "*** Reading source directory: " << source_dir << std::endl;
	std::cout << std::endl;
	
	///First check if cropping background is required
	if(flag_crop_bg){
		boost_fs::path path_processedData_root(source_dir);
		processedData_dir_name = processedData_dir + slash + path_processedData_root.leaf().c_str();
		file_ref_name = processedData_dir_name + slash + file_ref_basename;
		bg_dir_name =  processedData_dir_name + slash + str_bg;
		boost_fs::path path_bg_dir(bg_dir_name);
		if(!boost_fs::exists(path_bg_dir)){
			std::cout << "No bg folder located. Exiting out" << std::endl;
		}else{
			fstore_file_ref = cv::FileStorage(file_ref_name, cv::FileStorage::READ);
			bg_count = (int)fstore_file_ref["noBgImages"];
			fstore_file_ref.release();
			iterate_dir_bg(boost_fs::path(source_dir));
			fstore_file_ref = cv::FileStorage(file_ref_name, cv::FileStorage::APPEND);
			fstore_file_ref << "noBgImages" << bg_count;
			fstore_file_ref.release();
		}
	}else{
	
		///Create necessary files and directories
		//(1) create directory for the used database in the processed folder
		boost_fs::path path_processedData_root(source_dir);
		processedData_dir_name = processedData_dir + slash + path_processedData_root.leaf().c_str();
		boost_fs::path path_processedData_dir(processedData_dir_name);
		if(!boost_fs::exists(path_processedData_dir)){
			if(boost_fs::create_directory( boost_fs::path(processedData_dir_name) )){
				std::cout << "*** Processed Data directory created sucessfully" << std::endl;	
				std::cout << std::endl;	
			}
		}else{
			std::cout << "*** Existing Processed Data directory" << std::endl;
			std::cout << std::endl;
		}
		
		//(2)create directory for the patches if non-existing
		patches_dir_name = processedData_dir_name + slash + str_patches;
		boost_fs::path path_patches_dir(patches_dir_name);
		if(!boost_fs::exists(path_patches_dir)){
			if(boost_fs::create_directory( boost_fs::path(patches_dir_name) )){
				std::cout << "*** Patches directory created sucessfully" << std::endl;
				std::cout << patches_dir_name << std::endl;
				std::cout << std::endl;
			}
		}else{
			std::cout << "*** Existing patches directory" << std::endl;
			std::cout << patches_dir_name << std::endl;
			std::cout << std::endl;
		}
		
		//(3)create directory for image masks
		masks_dir_name =  processedData_dir_name + slash + str_masks;
		boost_fs::path path_masks_dir(masks_dir_name);
		if(!boost_fs::exists(path_masks_dir)){
			if(boost_fs::create_directory( boost_fs::path(masks_dir_name) )){
				std::cout << "*** Masks directory created sucessfully" << std::endl;
				std::cout << masks_dir_name << std::endl;
				std::cout << std::endl;
			}
		}else{
			std::cout << "*** Existing masks directory" << std::endl;
			std::cout << masks_dir_name << std::endl;
			std::cout << std::endl;
		}
		
		//(4)create a directory for background images
		bg_dir_name =  processedData_dir_name + slash + str_bg;
		boost_fs::path path_bg_dir(bg_dir_name);
		if(!boost_fs::exists(path_bg_dir)){
			if(boost_fs::create_directory( boost_fs::path(bg_dir_name) )){
				std::cout << "*** Background directory created sucessfully" << std::endl;
				std::cout << bg_dir_name << std::endl;
				std::cout << std::endl;
			}
		}else{
			std::cout << "*** Existing background directory" << std::endl;
			std::cout << bg_dir_name << std::endl;
			std::cout << std::endl;
		}	
		
		///Create file reference file to keep track of all processed and created files
		dir_tree_name = processedData_dir_name + slash + dir_tree_basename;
		file_inputs_name = processedData_dir_name + slash + file_inputs_basename;
		file_ref_name = processedData_dir_name + slash + file_ref_basename;
		
		if(!flag_continue_crop){
			fstore_file_ref = cv::FileStorage(file_ref_name, cv::FileStorage::WRITE);
			fstore_file_ref << "ProcessedImages" << "[";
			
			///Iterate through the directory recursively
			ptr_fstore = &fstore_file_ref;
			iterate_dir(boost_fs::path(source_dir), masks_dir_name);
			
			///Close file reference file
			fstore_file_ref << "]";
			fstore_file_ref << "noPatches" << patch_count;
			fstore_file_ref << "noBgImages" << bg_count;
			fstore_file_ref.release();
		}else{
			
			///First copy old data into the new ref file, and continue from there
			std::string file_ref_tmp_name = processedData_dir_name + slash + file_ref_tmp_basename;
			fstore_file_ref_tmp = cv::FileStorage(file_ref_tmp_name, cv::FileStorage::WRITE);
			fstore_file_ref_tmp << "ProcessedImages" << "[";
			
			fstore_file_ref = cv::FileStorage(file_ref_name, cv::FileStorage::READ);
			cv::FileNode process_im_node = fstore_file_ref["ProcessedImages"];
			cv::FileNodeIterator node_it = process_im_node.begin(), it_end = process_im_node.end();
			for(; node_it != it_end; ++node_it){
				std::string tmp_str;
				fstore_file_ref_tmp << "{";
				(*node_it)["input_image"] >> tmp_str;
				fstore_file_ref_tmp << "input_image" << tmp_str; tmp_str.clear();
				(*node_it)["patches_mask"] >> tmp_str;
				fstore_file_ref_tmp << "patches_mask" << tmp_str; tmp_str.clear();
				fstore_file_ref_tmp << "patches" << "[";
				
				cv::FileNode patches_node = (*node_it)["patches"];
				cv::FileNodeIterator node_it2 = patches_node.begin(), it_end2 = patches_node.end();
				for(; node_it2 != it_end2; ++node_it2){
					fstore_file_ref_tmp << "{";
					(*node_it2)["patch_name"] >> tmp_str;
					fstore_file_ref_tmp << "patch_name" << tmp_str; tmp_str.clear();
					fstore_file_ref_tmp << "center_x" << (int)(*node_it2)["center_x"];
					fstore_file_ref_tmp << "center_y" << (int)(*node_it2)["center_y"];
					fstore_file_ref_tmp << "height" << (int)(*node_it2)["height"];
					fstore_file_ref_tmp << "width" << (int)(*node_it2)["width"];
					fstore_file_ref_tmp << "}";
				}
				
				fstore_file_ref_tmp << "]";
				fstore_file_ref_tmp << "}";
			}
			
			///Iterate through the directory recursively
			std::cout << std::endl;
			std::cout << "Finish updating file_ref" << std::endl;
			std::cout << std::endl;
			
			patch_count = (int)fstore_file_ref["noPatches"];
			bg_count = (int)fstore_file_ref["noBgImages"];
			ptr_fstore = &fstore_file_ref_tmp;
			iterate_dir(boost_fs::path(source_dir), masks_dir_name);
			fstore_file_ref_tmp << "]";
			fstore_file_ref_tmp << "noPatches" << patch_count;
			fstore_file_ref_tmp << "noBgImages" << bg_count;
			
			fstore_file_ref.release();
			fstore_file_ref_tmp.release();
			
			///Remove and rename appropiate files
			boost_fs::remove(boost_fs::path(file_ref_name));
			boost_fs::rename(boost_fs::path(file_ref_tmp_name),boost_fs::path(file_ref_name));
			
		}
		
		if(flag_user_stop){
			std::cout << std::endl;
			std::cout << "Not all images were processed" << std::endl;
		}else{
			std::cout << std::endl;
			std::cout << "All images were processed" << std::endl;
		}
	
	}
	
	
	return 0;
}

