#include "JudgeOutput.h"
#include <iostream>
#include <regex>

void splitStr(const std::string &s, char delim, std::vector<std::string> &elems) {
	std::stringstream ss;
	ss.str(s);
	std::string item;
	while (std::getline(ss, item, delim)) {
		elems.push_back(item);
	}
}

/*
 * Check if images contains img
 * Return true if, false otherwise
 */
bool contains(std::vector<Image> &images, Image img) {
    std::vector<Image>::iterator it;
    for(it=images.begin() ; it < images.end(); it++) {
        if (((*it).la == img.la)&&((*it).lo == img.lo)) {
            return true;
        }
    }
    return false;
}

/*
 * Check if an image can be taken by a satelite
*/
bool isInRange(Satelite * sat, Image * im) {
	return (std::abs(sat->la - im->la) <= sat->maxRot) && (std::abs(sat->lo - im->lo) < sat->maxRot);
}

/*
 * Get the satelite from arraySat with his id
 * return ther satelite
*/
Satelite * getSat(Satelite * arraySat, int nbSat, int id) {
    Satelite * sat;
    for (int i = 0; i < nbSat; i++) {
        sat = &arraySat[i];
        if (sat->id == id) {
            return sat;
        }
    }
    std::cout << "nullptr" << std::endl;
    return nullptr;
}

Satelite * getSatPosition(Satelite * s,int turn)
{
    Satelite * sat;
    sat->maxRot=s->maxRot;
    sat->speed=s->speed;
    sat->speedRot=s->speedRot;
    if((s->la+s->speed)>=-324000&&(s->la+s->speed)>=-324000)
        {
            sat->la=s->la+turn*s->speed;
            sat->lo=s->lo-turn*15;
        }
    else if((s->la+s->speed)>324000)
        {
            sat->la=648000-(s->la+turn*s->speed);
            sat->lo=s->lo-turn*15-648000;
        }
    else
        {
            sat->la=-648000-(s->la+turn*s->speed);
            sat->lo=s->lo-turn*15-648000;
        }
    return sat;
}


/*
 * Contructor 
 * filename : path of the output file
*/
JudgeOutput::JudgeOutput(std::string outFileName, SimulationData * s) {
    outputFile = new std::ifstream(outFileName);
    this->simData = s;
}

/*
 * Check if the output file is valid :
    * The format of the output file matches the description.
    * Each picture is in range of the corresponding satellite when it’s taken.
    * No satellite moves the camera faster than w arcseconds per turn between taking two consecutive pictures.
 * Return true if valid, false otherwise
*/
bool JudgeOutput::isValidOutput() {
    return ((isValidSyntax())&&(isValidImages()));
}

/* 
 * Check if the file format syntax is valid. Check the semantic too
 * maxPictures: the number of pictures of the input file
 * maxTurns: number of the turn of the simulation
 * satNb: number of satelite in the simulation
 * Return true if valid, false otherwise
*/
bool JudgeOutput::isValidSyntax() {
    long maxTurns = simData->getDuration();
    int satNb = simData->getNbSatelite();
    int maxPictures = simData->getNbCollection();


	// ----- SYNTAX CHECK -----

	// Definition of usefull regular expressions.
	std::regex number_regex("[[:digit:]]+");
	std::regex line_regex("-?([[:digit:]]+[[:space:]]){3}[[:digit:]]+");

	// Get of the first line
	std::string line;
	std::getline(*outputFile, line);

	// Test of the first line
	if (!std::regex_match(line, number_regex)) {
		// Test failed.
		std::cout << "[E] Test failed at line : '" << line << "'." << std::endl;
		return false;
	}

	// Convert number after check pass
	int picturesNb = std::stoi(line);

	// Creation of the picture array
	std::string * pictureArray = new std::string[picturesNb];

	// Test of each line
	int iter = 0;
	while (std::getline(*outputFile, line)) {
		if (!std::regex_match(line, line_regex)) {
			// Test failed.
			std::cout << "[E] Test failed at line : '" << line << "'." << std::endl;
			return false;
		}
		else {
			// Test success : line saved.
			pictureArray[iter] = line;
			iter++;
		}
	}


	// ------- SEMANTICAL CHECK --------

	// Test of the number of pictures
	if (picturesNb > maxPictures || picturesNb <= 0) {
		return false;
	}

	std::vector<std::string> elems;
	int la, lo, tu, sa;

	// Test of each line
	for (int i = 0; i < picturesNb; i++)
	{
		splitStr(pictureArray[i], ' ', elems);
		try
		{
			la = std::stoi(elems[0]);
			lo = std::stoi(elems[1]);
			tu = std::stoi(elems[2]);
			sa = std::stoi(elems[3]);
		}
		catch (const std::exception&)
		{
			return false;
		}
		if ((la < -306000) || (la > 306000) || (lo < -648000) || (lo > 647999) || (tu > maxTurns) || (tu < 0) || (sa+1 > satNb) || (sa < 0)) {
			return false;
		}
	}
	return true;
}

/*
 * Check all the images of the output
 * Return true if all images are valid, false otherwise
*/
bool JudgeOutput::isValidImages() {
    Image * img;
    Satelite * sat;
    int turn;
    std::string line;

    // Get the array satelite and number of it
    Satelite * arraySat = this->simData->getArraySat();
    long nbSatelite = this->simData->getNbSatelite();

     // Get the first line : number of image taken
	std::getline(*outputFile, line);
    std::vector<std::string> elems;
	int nbImageTaken = std::stoi(line);

    // To compare in the for (first id is 0)
    sat->id = -1;

    for (int i = 0; i < nbImageTaken; i++) {
        img = new Image();
        std::getline(*outputFile, line);

        splitStr(line, ' ', elems);

        img->la = std::stoi(elems[0]);
        img->lo = std::stoi(elems[1]);

        turn = std::stoi(elems[2]);

        // Change the satelite only if it different
        if (sat->id != std::stoi(elems[3])) {
            // Get the satelite from arraySat with his ID
            sat = getSat(arraySat, nbSatelite, std::stoi(elems[3]));
        }

         // Test the current image
        if (!isValidImage(turn, img, sat)) {
            return false;
        }
        elems.clear();
    }
    // Get back to the beginning of the file
    outputFile->clear();
    outputFile->seekg(0, std::ios::beg);
    return true;
}

/*
 * Check if the image img was in the range of the satelite sat at turn t
 * Return true if it was in range, false otherwise
*/
bool JudgeOutput::isValidImage(int t, Image * img, Satelite * sat) {
    return isInRange(getSatPosition(sat, t), img);
}

/*
 * Get all the images taken from the outputFile
 * Return a vector of all the images
*/
std::vector<Image> JudgeOutput::getImagesTaken() {
    std::vector<Image> imgs;
    Image * img;
    std::vector<std::string> elems;
	std::string line;

    // Get the first line : number of image taken
	std::getline(*outputFile, line);
	int nbImageTaken = std::stoi(line);

    // Browse all the images 
    for (int i = 0; i < nbImageTaken; i++) {
        img = new Image();

        std::getline(*outputFile, line);
        splitStr(line, ' ', elems);

        img->la = std::stoi(elems[0]);
        img->lo = std::stoi(elems[1]);

        imgs.push_back(*img);
        elems.clear();
    }
    outputFile->clear();
    outputFile->seekg(0, std::ios::beg);
    return imgs;
}

bool JudgeOutput::isValidCamera()
{
    return false;
}

/*
 * Get the score of the simulation 
 * Return the score
*/
int JudgeOutput::getScore() {
    int score = 0;
    
    int numCol;
    int numImg;
    bool colComplete;
    Image img;

    std::vector<Image> images = this->getImagesTaken();
    Collection * arrayCol = this->simData->getArrayCol();
    long nbCol = this->simData->getNbCollection();

    //  Browse the collection array 
    for (numCol = 0; numCol < nbCol; numCol++) { 
        Collection col = arrayCol[numCol];
        colComplete = true;
      //  std::cout << "Nb img : " << col.nbImg << std::endl;
        // Browse pictures of collection col
        for (numImg = 0; numImg < col.nbImg; numImg++) {
            img = col.listImg[numImg];
           // std::cout << "Col " << numCol << ", img " << numImg << " : " << img.la << " " << img.lo;
           
            // Check if img was taken
            if (!contains(images, img)) {
                colComplete = false;
                //std::cout << std::endl;
            }
            else  {
               // std::cout << " _/" << std::endl;
            }
        }
        // If col complete : add the score of col
        if (colComplete) {
            score += col.nbPts;
            //std::cout << "Collection " << numCol << " complete" << std::endl;
        }
    }
    return score;
}
