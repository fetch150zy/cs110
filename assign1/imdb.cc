#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <algorithm>
#include "imdb.h"
using namespace std;

const char *const imdb::kActorFileName = "actordata";
const char *const imdb::kMovieFileName = "moviedata";
imdb::imdb(const string& directory) {
  const string actorFileName = directory + "/" + kActorFileName;
  const string movieFileName = directory + "/" + kMovieFileName;  
  actorFile = acquireFileMap(actorFileName, actorInfo);
  movieFile = acquireFileMap(movieFileName, movieInfo);
}

bool imdb::good() const {
  return !( (actorInfo.fd == -1) || 
	    (movieInfo.fd == -1) ); 
}

imdb::~imdb() {
  releaseFileMap(actorInfo);
  releaseFileMap(movieInfo);
}

bool imdb::getCredits(const string& player, vector<film>& films) const { 
  int total_actor_num = *(int*) actorFile;
  int* actor_base_ptr = ((int*) actorFile) + 1;

  auto less = [=](const int offset, const string cstr) {
    char* value = ((char*) actorFile) + offset; 
    return value < cstr;	
  };
  auto equals = [=](const int offset, const string cstr) {
    char* value = ((char*) actorFile) + offset; 
    return value == cstr;	
  };
  auto first = std::lower_bound(actor_base_ptr, actor_base_ptr + total_actor_num, player, less);
  auto it = (first != (actor_base_ptr + total_actor_num) && equals(*first, player)) ? first : (actor_base_ptr + total_actor_num);

  if(it != actor_base_ptr + total_actor_num) {
    int actor_idx = it - actor_base_ptr;
    int offset = *(actor_base_ptr + actor_idx);

    char* record_base_ptr = ((char*) actorFile) + offset;
    int name_offset = strlen(record_base_ptr);
    if(name_offset % 2 == 0)
      name_offset += 2;
    else
      name_offset += 1;

    short movie_num = *(short*)(record_base_ptr + name_offset);
    int name_num_offset = name_offset + 2;
    if(name_num_offset % 4 == 2)
      name_num_offset += 2;

    int* movie_base_ptr = (int*)(record_base_ptr + name_num_offset);
    for(int i = 0; i < movie_num; i++) {
      int movie_offset = *(movie_base_ptr + i);
      char* movie_name = ((char*) movieFile) + movie_offset;
      int movie_name_offset = strlen(movie_name) + 1;
      int movie_year = ((int)*(unsigned char*)(movie_name + movie_name_offset)) + 1900;

      film temp_film;
      temp_film.title = movie_name;
      temp_film.year = movie_year;
      films.push_back(temp_film);
    }
    return true;
  }
return false;
}

bool imdb::getCast(const film& movie, vector<string>& players) const { 
  int total_movie_num = *((int*) movieFile);
  int* movie_base_ptr = ((int*) movieFile) + 1;

  auto less = [=](const int offset, const film fi) {
	char* movie_name = ((char*) movieFile) + offset; 
	unsigned movie_name_offset = strlen(movie_name) + 1;
	unsigned movie_year = ((unsigned)*(unsigned char*)(movie_name + movie_name_offset)) + 1900;	
	film film_temp;
	film_temp.title = movie_name;
	film_temp.year = movie_year;
	return film_temp < fi;	
  };
  auto equals = [=](const int offset, const film fi) {
	char* movie_name = ((char*) movieFile) + offset; 
	unsigned movie_name_offset = strlen(movie_name) + 1;
	unsigned movie_year = ((unsigned)*(unsigned char*)(movie_name + movie_name_offset)) + 1900;	
    film film_temp;
    film_temp.title = movie_name;
    film_temp.year = movie_year;
    return film_temp == fi;	
  };
  auto first = std::lower_bound(movie_base_ptr, movie_base_ptr + total_movie_num, movie, less);
  auto it = (first != (movie_base_ptr + total_movie_num) && equals(*first, movie)) ? first : (movie_base_ptr + total_movie_num);
	
  if(it != movie_base_ptr + total_movie_num) {
	int movie_idx = it - movie_base_ptr;
	int offset = *(movie_base_ptr + movie_idx);
	char* record_base_ptr = ((char*) movieFile) + offset;
	int name_offset = strlen(record_base_ptr) + 1;
	int name_year_offset = name_offset + 1;
    if(name_year_offset % 2 == 1)
	  name_year_offset += 1;

    short actor_num = *(short*)(record_base_ptr + name_year_offset);
    int name_year_num_offset = name_year_offset + 2;
    if(name_year_num_offset % 4 == 2)
      name_year_num_offset += 2;

    int* actor_base_ptr = (int*)(record_base_ptr + name_year_num_offset);
    for(int i = 0; i < actor_num; i++) {
      int actor_offset = *(actor_base_ptr + i);
      char* actor_name = ((char*) actorFile) + actor_offset;
      players.push_back(actor_name);
    }
    return true;
  }
  return false; 
}

const void *imdb::acquireFileMap(const string& fileName, struct fileInfo& info) {
  struct stat stats;
  stat(fileName.c_str(), &stats);
  info.fileSize = stats.st_size;
  info.fd = open(fileName.c_str(), O_RDONLY);
  return info.fileMap = mmap(0, info.fileSize, PROT_READ, MAP_SHARED, info.fd, 0);
}

void imdb::releaseFileMap(struct fileInfo& info) {
  if (info.fileMap != NULL) munmap((char *) info.fileMap, info.fileSize);
  if (info.fd != -1) close(info.fd);
}
