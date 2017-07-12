#include <string>
#include <string.h>
#include <vector>
#include <fstream>
#include <dirent.h>
#include <unistd.h>
#include <iostream>
#include "crts.hpp"

struct ce_info{
  std::string ce_name;
  std::string ce_dir;
};

int num_ces = 0;
int num_srcs = 0;
int num_headers = 0;
struct ce_info ces[100];
std::string src_list[100];
std::string header_list[100];

void process_directory(std::string directory, bool customize, bool ce_dir){

  printf("Processing directory: %s\n", directory.c_str());
  DIR *dpdf;
  struct dirent *epdf;
  bool useThisCE;
  bool ce_src_found = false;
  int ce_ind = (num_ces>0) ? num_ces-1 : 0; // store the current ce count so it can be used later

  std::string src_file_name = "";
          
  // read directory contents
  dpdf = opendir(directory.c_str());
  if (dpdf != NULL) {
    while ((epdf = readdir(dpdf)) != NULL) {
      
      // handle subdirectories
      if ((epdf->d_type == DT_DIR) && strcmp(epdf->d_name,".") && strcmp(epdf->d_name,"..")){ 
        // verify length
        if (strlen(epdf->d_name) >= 3) {
          // subdirectory defines a CE
          if (epdf->d_name[0] == 'C' && epdf->d_name[1] == 'E' &&
              epdf->d_name[2] == '_') {

            useThisCE = true;
            if (customize){
              char input;
              do {
                std::cout << "Use " << (directory+"/"+epdf->d_name) <<"? [y/n]" << std::endl;
                std::cin >> input;
              } while (!std::cin.fail() && input!='y' && input!='n');
              if (input == 'n')
                useThisCE = false;
            }
            if (useThisCE) {
              // Copy filename and path into list of CEs
              ces[num_ces].ce_name.assign(epdf->d_name);
              ces[num_ces].ce_dir.assign(directory+"/"+epdf->d_name);
              num_ces++;
            } 
          
            process_directory(directory+"/"+epdf->d_name, customize, true);
          } else /* non-CE subdirectory (doesn't start with 'CE_')*/{
            process_directory(directory+"/"+epdf->d_name, customize, false);
          }
        } else /* non-CE subdirectory (short name)*/{
          process_directory(directory+"/"+epdf->d_name, customize, false);
        }
      }

      // handle standard file
      if (epdf->d_type == DT_REG){
        // verify length
        if (strlen(epdf->d_name) >= 3) {
          // file is the CE source file
          std::string ce_src = ces[ce_ind].ce_name + ".cpp";
          if (ce_dir && (!strcmp(epdf->d_name, ce_src.c_str()))) {
            ce_src_found = true;
          }
          // file is non-CE source file 
          else if ((epdf->d_name[strlen(epdf->d_name) - 2] == '.' &&
                    epdf->d_name[strlen(epdf->d_name) - 1] == 'c') || 
                   (epdf->d_name[strlen(epdf->d_name) - 3] == '.' &&
                    epdf->d_name[strlen(epdf->d_name) - 2] == 'c' &&
                    epdf->d_name[strlen(epdf->d_name) - 1] == 'c') ||
                   (epdf->d_name[strlen(epdf->d_name) - 4] == '.' &&
                    epdf->d_name[strlen(epdf->d_name) - 3] == 'c' &&
                    epdf->d_name[strlen(epdf->d_name) - 2] == 'p' &&
                    epdf->d_name[strlen(epdf->d_name) - 1] == 'p')) {
            bool useThisSrc = true;
            if (customize)
            {
              char input;
              do {
                std::cout << "Use " << (directory+"/"+epdf->d_name) <<"? [y/n]" << std::endl;
                std::cin >> input;
              } while (!std::cin.fail() && input!='y' && input!='n');
              if (input == 'n')
                useThisSrc = false;
            }
            if (useThisSrc) {
              // Copy filename into list of src names for specific ce
              src_list[num_srcs].assign(directory+"/"+epdf->d_name);
              num_srcs++;
            }
          }
          // file is non-CE header file 
          else if ((epdf->d_name[strlen(epdf->d_name) - 2] == '.' &&
                    epdf->d_name[strlen(epdf->d_name) - 1] == 'h') || 
                   (epdf->d_name[strlen(epdf->d_name) - 3] == '.' &&
                    epdf->d_name[strlen(epdf->d_name) - 2] == 'h' &&
                    epdf->d_name[strlen(epdf->d_name) - 1] == 'h') ||
                   (epdf->d_name[strlen(epdf->d_name) - 4] == '.' &&
                    epdf->d_name[strlen(epdf->d_name) - 3] == 'h' &&
                    epdf->d_name[strlen(epdf->d_name) - 2] == 'p' &&
                    epdf->d_name[strlen(epdf->d_name) - 1] == 'p')) {
            bool useThisSrc = true;
            if (customize)
            {
              char input;
              do {
                std::cout << "Use " << (directory+"/"+epdf->d_name) <<"? [y/n]" << std::endl;
                std::cin >> input;
              } while (!std::cin.fail() && input!='y' && input!='n');
              if (input == 'n')
                useThisSrc = false;
            }
            if (useThisSrc) {
              // Copy filename into list of src names for specific ce
              header_list[num_headers].assign(directory+"/"+epdf->d_name);
              num_headers++;
            }
          }
        }
      }
    }
  }

  // make sure we've found the ce source file if in a ce directory
  if ( ce_dir && (!ce_src_found)) {
    printf("The source file was not found for the CE: %s%s%s\n", ces[ce_ind].ce_dir.c_str(), "/", ces[ce_ind].ce_name.c_str());
    exit(EXIT_FAILURE);
  }
}

void help_config_cognitive_engines() {
  printf("config_CEs -- Configure CRTS to use cusotm cognitive engines\n");
  printf("              (located in the cognitive_engines/ directory).\n");
  printf(" -h : Help.\n");
  printf(" -c : Customize the selection of cognitive engines and source files.\n");
}

int main(int argc, char **argv) {

  printf("Consult the CRTS-Manual.pdf if you experience trouble configuring your CE's\n\n");
  
  // Default options
  bool customize = false;

  // Process options
  int opt;
  while ((opt = getopt(argc, argv, "hc")) != EOF ) {
    switch(opt) {
      case 'c':
        customize = true;
        break;
      case 'h':
        help_config_cognitive_engines();
        exit(EXIT_SUCCESS);
      default:
        help_config_cognitive_engines();
        exit(EXIT_FAILURE);
    }
  }

  // Launch recursive process which reads in info for all CE's in
  // base directory and any subdirectories
  std::string base_directory = "cognitive_engines";
  process_directory(base_directory, customize, false);

  printf("\nConfiguring CRTS to use the following cognitive engines:\n\n");
  for (int i = 0; i < num_ces; i++)
    printf("%s\n", ces[i].ce_name.c_str());

  printf("\nThe following files will be included as additional sources:\n\n");
  for (int i = 0; i < num_srcs; i++)
    printf("%s\n", src_list[i].c_str());
  printf("\n");

  // create string vector
  std::vector<std::string> file_lines;
  std::string line;
  std::string flag_beg;
  std::string flag_end;
  bool edit_content = false;

  //////////////////////////////////////////////////////////////////////////////////
  // Edit ECR.cpp
  //////////////////////////////////////////////////////////////////////////////////

  flag_beg = "EDIT SET CE START FLAG";
  flag_end = "EDIT SET CE END FLAG";

  // open read file
  std::ifstream file_in("src/extensible_cognitive_radio.cpp", std::ifstream::in);

  // read file until the end
  while (!(file_in.eof())) {

    std::getline(file_in, line);
    if (!edit_content) {
      file_lines.push_back(line); // push all lines on vector
      std::size_t found = line.find(flag_beg);
      if (found != std::string::npos) {
        edit_content = true;

        // push all lines to map subclass
        for (int i = 0; i < num_ces; i++) {
          line = "  if(!strcmp(ce, \"" + ces[i].ce_name + "\"))";
          file_lines.push_back(line);
          line = "    CE = new " + ces[i].ce_name + "(argc, argv, this);";
          file_lines.push_back(line);
        }
      }
    }
    // else delete all lines until end flag is found
    else {
      // if end flag is found push it onto the vector and go back to standard
      // mode
      std::size_t found = line.find(flag_end);
      if (found != std::string::npos) {
        file_lines.push_back(line);
        edit_content = false;
      }
    }
  }
  // close read file
  file_in.close();

  // write file
  std::ofstream file_out("src/extensible_cognitive_radio.cpp", std::ofstream::out);
  for (std::vector<std::string>::iterator i = file_lines.begin();
       i != file_lines.end(); i++) {
    file_out << (*i);
    if (i != file_lines.end() - 1)
      file_out << '\n';
  }
  file_out.close();

  /////////////////////////////////////////////////////////////////////////////////////
  // Edit CE.hpp
  /////////////////////////////////////////////////////////////////////////////////////

  flag_beg = "EDIT INCLUDE START FLAG";
  flag_end = "EDIT INCLUDE END FLAG";

  file_lines.clear();

  // open read file
  file_in.open("src/extensible_cognitive_radio.cpp", std::ifstream::in);

  // read file until the end
  while (!(file_in.eof())) {

    std::getline(file_in, line);
    if (!edit_content) {
      file_lines.push_back(line); // push all lines on vector
      std::size_t found = line.find(flag_beg);
      // start to edit content once start flag is found
      if (found != std::string::npos) {
        edit_content = true;

        // push all lines to map subclass
        std::string line_new;
        for (int i = 0; i < num_ces; i++) {
          line_new = "#include \"../"+ces[i].ce_dir+"/"+ces[i].ce_name+".hpp\"";
          file_lines.push_back(line_new);
        }
      }
    }
    // else delete all lines until end flag is found
    else {
      // if end flag is found push it onto the vector and go back to standard
      // mode
      std::size_t found = line.find(flag_end);
      if (found != std::string::npos) {
        file_lines.push_back(line);
        edit_content = false;
      }
    }
  }
  // close read file
  file_in.close();

  // write file
  file_out.open("src/extensible_cognitive_radio.cpp", std::ofstream::out);
  for (std::vector<std::string>::iterator i = file_lines.begin();
       i != file_lines.end(); i++) {
    file_out << (*i);
    if (i != file_lines.end() - 1)
      file_out << '\n';
  }
  file_out.close();

  /////////////////////////////////////////////////////////////////////////////
  // Edit makfile CE object list
  /////////////////////////////////////////////////////////////////////////////

  file_lines.clear();

  flag_beg = "EDIT CE OBJECT LIST START FLAG";
  flag_end = "EDIT CE OBJECT LIST END FLAG";

  // open header file
  file_in.open("makefile", std::ifstream::in);

  // read file until the end
  while (!(file_in.eof())) {

    std::getline(file_in, line);
    if (!edit_content) {
      file_lines.push_back(line); // push all lines on vector
      std::size_t found = line.find(flag_beg);
      // start to edit content once start flag is found
      if (found != std::string::npos) {
        edit_content = true;

        // push all lines to map subclass
        std::string line_new;
        line_new = "CEs = src/cognitive_engine.cpp ";
        for (int i = 0; i < num_ces; i++) {
          line_new += ("lib/"+ces[i].ce_name+".o ");
        }
        file_lines.push_back(line_new);
        line_new = "CE_srcs = ";
        for (int i = 0; i < num_srcs; i++) {
          line_new += (" "+src_list[i]);
        }
        file_lines.push_back(line_new);

        line_new = "CE_Headers = ";
        for (int i = 0; i < num_ces; i++) {
          line_new += ces[i].ce_dir+"/"+ces[i].ce_name+".hpp ";
        }
        for (int i = 0; i < num_headers; i++) {
          line_new += header_list[i]+" ";
        }
        file_lines.push_back(line_new);
      }
    }
    // else delete all lines until end flag is found
    else {
      // if end flag is found push it onto the vector and go back to standard
      // mode
      std::size_t found = line.find(flag_end);
      if (found != std::string::npos) {
        file_lines.push_back(line);
        edit_content = false;
      }
    }
  }
  // close read file
  file_in.close();

  // write file
  file_out.open("makefile", std::ofstream::out);
  for (std::vector<std::string>::iterator i = file_lines.begin();
       i != file_lines.end(); i++) {
    file_out << (*i);
    if (i != file_lines.end() - 1)
      file_out << '\n';
  }
  file_out.close();
  
  /////////////////////////////////////////////////////////////////////////////
  // Edit makfile CE compilation
  /////////////////////////////////////////////////////////////////////////////

  file_lines.clear();

  flag_beg = "EDIT CE COMPILATION START FLAG";
  flag_end = "EDIT CE COMPILATION END FLAG";

  // open header file
  file_in.open("makefile", std::ifstream::in);

  // read file until the end
  while (!(file_in.eof())) {

    std::getline(file_in, line);
    if (!edit_content) {
      file_lines.push_back(line); // push all lines on vector
      std::size_t found = line.find(flag_beg);
      // start to edit content once start flag is found
      if (found != std::string::npos) {
        edit_content = true;

        // push all lines to map subclass
        std::string line_new; 
        for (int i = 0; i < num_ces; i++) {
          line_new = "lib/"+ces[i].ce_name+".o: ";
          line_new += (ces[i].ce_dir+"/"+ces[i].ce_name+".cpp ");
          line_new += (ces[i].ce_dir+"/"+ces[i].ce_name+".hpp ");
          for (int i = 0; i < num_srcs; i++) {
            //line_new += " cognitive_engines/";
            line_new += (src_list[i]+" ");
          }
          file_lines.push_back(line_new);

          line_new = "\tg++ $(FLAGS) -c -o lib/"+ces[i].ce_name+".o ";
          line_new += (ces[i].ce_dir+"/"+ces[i].ce_name+".cpp ");
          file_lines.push_back(line_new);
          line_new = "";
          file_lines.push_back(line_new);
        }      
      }
    }
    // else delete all lines until end flag is found
    else {
      // if end flag is found push it onto the vector and go back to standard
      // mode
      std::size_t found = line.find(flag_end);
      if (found != std::string::npos) {
        file_lines.push_back(line);
        edit_content = false;
      }
    }
  }
  // close read file
  file_in.close();

  // write file
  file_out.open("makefile", std::ofstream::out);
  for (std::vector<std::string>::iterator i = file_lines.begin();
       i != file_lines.end(); i++) {
    file_out << (*i);
    if (i != file_lines.end() - 1)
      file_out << '\n';
  }
  file_out.close();
 
  return 0;
}
