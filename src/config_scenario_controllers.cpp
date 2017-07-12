#include <string>
#include <string.h>
#include <vector>
#include <fstream>
#include <dirent.h>
#include <unistd.h>
#include <iostream>
#include "crts.hpp"

struct sc_info{
  std::string sc_name;
  std::string sc_dir;
};

int num_scs = 0;
int num_srcs = 0;
int num_headers = 0;
struct sc_info scs[100];
std::string src_list[100];
std::string header_list[100];

void process_directory(std::string directory, bool customize, bool sc_dir){

  printf("Processing directory: %s\n", directory.c_str());
  DIR *dpdf;
  struct dirent *epdf;
  bool useThisSC;
  bool sc_src_found = false;
  int sc_ind = (num_scs>0) ? num_scs-1 : 0; // store the current ce count so it can be used later

  std::string src_file_name = "";
          
  // read directory contents
  dpdf = opendir(directory.c_str());
  if (dpdf != NULL) {
    while ((epdf = readdir(dpdf)) != NULL) {
      
      // handle subdirectories
      if ((epdf->d_type == DT_DIR) && strcmp(epdf->d_name,".") && strcmp(epdf->d_name,"..")){ 
        // verify length
        if (strlen(epdf->d_name) >= 3) {
          // subdirectory defines a SC
          if (epdf->d_name[0] == 'S' && epdf->d_name[1] == 'C' &&
              epdf->d_name[2] == '_') {

            useThisSC = true;
            if (customize){
              char input;
              do {
                std::cout << "Use " << (directory+"/"+epdf->d_name) <<"? [y/n]" << std::endl;
                std::cin >> input;
              } while (!std::cin.fail() && input!='y' && input!='n');
              if (input == 'n')
                useThisSC = false;
            }
            if (useThisSC) {
              // Copy filename and path into list of CEs
              scs[num_scs].sc_name.assign(epdf->d_name);
              scs[num_scs].sc_dir.assign(directory+"/"+epdf->d_name);
              num_scs++;
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
          // file is the SC source file
          std::string sc_src = scs[sc_ind].sc_name + ".cpp";
          if (sc_dir && (!strcmp(epdf->d_name, sc_src.c_str()))) {
            sc_src_found = true;
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
  if ( sc_dir && (!sc_src_found)) {
    printf("The source file was not found for the SC: %s%s%s\n", scs[sc_ind].sc_dir.c_str(), "/", scs[sc_ind].sc_name.c_str());
    exit(EXIT_FAILURE);
  }
}

void help_config_scenario_controllers() {
  printf("config_scenario_controllerss -- Configure CRTS to use custom scenario controllers\n");
  printf("                                (located in the scenario_controllers/ directory).\n");
  printf(" -h : Help.\n");
  printf(" -c : Customize the selection of scenario controllers and source files.\n");
}

int main(int argc, char **argv) {

  printf("Consult the CRTS-Manual.pdf if you experience trouble configuring your SC's\n\n");
  
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
        help_config_scenario_controllers();
        exit(EXIT_SUCCESS);
      default:
        help_config_scenario_controllers();
        exit(EXIT_FAILURE);
    }
  }

  // Launch recursive process which reads in info for all CE's in
  // base directory and any subdirectories
  std::string base_directory = "scenario_controllers";
  process_directory(base_directory, customize, false);

  printf("\nConfiguring CRTS to use the following scenario controllers:\n\n");
  for (int i = 0; i < num_scs; i++)
    printf("%s\n", scs[i].sc_name.c_str());

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
  // Edit set controller in CRTS_controller.cpp

  flag_beg = "EDIT SET SC START FLAG";
  flag_end = "EDIT SET SC END FLAG";

  // open read file
  std::ifstream file_in("src/crts_controller.cpp", std::ifstream::in);

  // read file until the end
  while (!(file_in.eof())) {

    std::getline(file_in, line);
    if (!edit_content) {
      file_lines.push_back(line); // push all lines on vector
      std::size_t found = line.find(flag_beg);
      if (found != std::string::npos) {
        edit_content = true;

        // push all lines to map subclass
        for (int i = 0; i < num_scs; i++) {
          line = "  if(!strcmp(sp->SC, \"" + scs[i].sc_name + "\"))";
          file_lines.push_back(line);
          line = "    SC = new " + scs[i].sc_name + "(argc, argv);";
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
  std::ofstream file_out("src/crts_controller.cpp", std::ofstream::out);
  for (std::vector<std::string>::iterator i = file_lines.begin();
       i != file_lines.end(); i++) {
    file_out << (*i);
    if (i != file_lines.end() - 1)
      file_out << '\n';
  }
  file_out.close();

  /////////////////////////////////////////////////////////////////////////////////////
  // Edit includes in CRTS_controller.cpp

  flag_beg = "EDIT INCLUDE START FLAG";
  flag_end = "EDIT INCLUDE END FLAG";

  file_lines.clear();

  // open read file
  file_in.open("src/crts_controller.cpp", std::ifstream::in);

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
        for (int i = 0; i < num_scs; i++) {
          line_new = "#include \"../"+scs[i].sc_dir+"/"+scs[i].sc_name+".hpp\"";
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
  file_out.open("src/crts_controller.cpp", std::ofstream::out);
  for (std::vector<std::string>::iterator i = file_lines.begin();
       i != file_lines.end(); i++) {
    file_out << (*i);
    if (i != file_lines.end() - 1)
      file_out << '\n';
  }
  file_out.close();

  ///////////////////////////////////////////////////////////////////////////////////////
  // Edit makfile

  file_lines.clear();

  flag_beg = "EDIT SC START FLAG";
  flag_end = "EDIT SC END FLAG";

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
        line_new = "SCs = src/scenario_controller.cpp ";
        for (int i = 0; i < num_scs; i++) {
          line_new += scs[i].sc_dir+"/"+scs[i].sc_name;
          line_new += ".cpp ";
        }
        for (int i = 0; i < num_srcs; i++) {
          line_new += src_list[i];
        }
        file_lines.push_back(line_new);

        line_new = "SC_Headers = ";
        for (int i = 0; i < num_scs; i++) {
          line_new += scs[i].sc_dir+"/"+scs[i].sc_name+".hpp ";
        }
        for (int i = 0; i < num_headers; i++) {
          line_new += header_list[i]+" ";
        }
// line_new += "\r";
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

  return 0;
}
