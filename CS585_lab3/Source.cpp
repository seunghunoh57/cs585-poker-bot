/**
CS585_Lab3.cpp
@author: Ajjen Joshi
@version: 1.0 9/17/2014

CS585 Image and Video Computing Fall 2014
Lab 3
--------------
This program introduces the following concepts:
a) Finding objects in a binary image
b) Filtering objects based on size
c) Obtaining information about the objects described by their contours
--------------
*/


#include "opencv2/core/core.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/objdetect.hpp"

#include <math.h>
#include <iostream>
#include <vector> 
#include <stdio.h>
#include <conio.h>
#include <stdlib.h>
#include <windows.h>

using namespace cv;
using namespace std;

//Global variables
int thresh = 128;
int max_thresh = 255;
int minSuiteIndex = -1;
double minSuiteMatrix = 765;
int minNumIndex = -1;
double minNumMatrix = 765;

String face_cascade_name = "haarcascade_frontalface_alt.xml";
String eyes_cascade_name = "haarcascade_eye_tree_eyeglasses.xml";
CascadeClassifier face_cascade;
CascadeClassifier eyes_cascade;
String window_name = "Capture - Face detection";
int handstrength;			//estimate of strength of computer's hand

int rating[13][4];			//2d array corresponding to cards in computer's hand + dealer's
//if the card is in the computer's hand, val = 2, if its on the board val = 1
//13 rows for 13 distinct card values, 4 cols for 4 suits
//2 = row index 0, ascending order as row index increases
//col 0 = spades, 1 = clubs, 2 = diamonds, 3 = hearts

String hara = "";			//computer's hand in string format
//r = royal flush
//u + number = sraight flush + high of that straight
//f + number = four of a kind, val of 4
//h + number + / + number = full house, val of set of 3, val of set of 2
//w + number = flush + highest card of flush
//s + number = straight + highest card in straight
//t + number = three of a kind, val of set of 3
//i + number  + , + number = two pairs, val of first pair, val of 2nd pair
//p + number = pair, val of pair
//a + number = high card, num of high card


// Method initialization
Mat retrieveSuiteTemplates(int ref);
Mat retrieveNumberTemplates(int ref);
string checkCardSuite(Mat&, Rect&, int i);
string checkCardNumber(Mat&, Rect&, int i);
Mat process(Mat& img);
double match(Mat& img, Mat& templ);
bool CardMotion(Mat& curr, Mat& prev);
void regCardRating(String card, int rat[13][4]);
int calcChipValue(Mat& curr);
void calcHand();
string detection(Mat& src);


// main function
int main()
{

	VideoCapture cap(0);
	if (!cap.isOpened())
	{
		cout << "Cannot open the video cam" << endl;
		return -1;
	}


	Mat src;		//curr and prev frame vars
	Mat tomcat;

	Rect indareaparam;
	Mat indarea;		//area to scan for player indication that next phase has started
	Mat indareaprev;

	Mat compchipsprev;	//prev pics of the chips in each area
	Mat oppchipsprev;
	Mat potprev;

	Mat curropcards;	//current and previous image of opponent's card area
	Mat prevopcards;

	String state = "w4h";		//what is the current stage of the game?
	//'w4h' = waiting for hand to be dealt, 'inibet' = initial betting before dealer reveals cards
	//'flop' = dealer reveals first 3 cards, 'turn' = fourth card revealed, 'river' = last card revealed
	//'dst' = distributing results of last round

	bool betting = false;		//true while betting back and forth

	bool reset = true;			//reset state variables

	int skipframes = 0;			//skip the next few frames after dectecting motion otherwise one hand wave will become multiple actions

	String pc1 = "";					//player cards
	String pc2 = "";

	int comphigh;				//highest value card in computer's hand
	int thresh = 0;				//threshold hand strength needed to not fold / for comp to keep betting with its hand

	int compchips;				//computer guesses for how many chips it has, opponent has, in pot
	int oppchips;
	int pot;

	bool nextstate = false;			//keep track of if the opponent folded

	bool callin = false;			//keeps track of if either player has gone all in -> to skip betting phase of later states
	bool pallin = false;

	int cardsrevealed = 0;			//keeps track of how many cards revealed

	bool cardsdealt = false;	//will be used to see if opponent folded
	//when someone folds game will be over (2 players only) so dealer
	//will collect computer's cards -> cardmotion returns true -> computer knows to move to next round

	//chips are red (value = 25), (blue v = 10), green (v = 5)

	int compchipval = 0;
	int oppchipval = 0;
	int potval = 0;

	int mylastbet;


	bool bSuccess0 = cap.read(src);
	if (!bSuccess0)
		cout << "Cannot read a frame from video stream" << endl;
	indareaparam = Rect(src.cols - 100, 0, 100, 100);
	
	indarea = src(indareaparam);
	indareaprev = src(indareaparam);
	
	tomcat = src.clone();
	

	while (1)
	{
		/*imshow("indarea", indarea);
		imshow("indareaprev", indareaprev);*/
		bool bSuccess = cap.read(src);
		if (!bSuccess0)
		{
			cout << "Cannot read a frame from video stream" << endl;
			break;
		}
		
		
		Rect playerPot, botPot, playerHand, mainPot, board;
		mainPot = Rect(50, 380, 100, 100);
		botPot = Rect(500, 380, 100, 100);
		playerHand = Rect(50, 200, 200, 80);
		playerPot = Rect(500, 200, 100, 80);
		board = Rect(0, 280, src.cols, 100);

		// Bounding boxes for pots, hand, and board

		cv::rectangle(src, mainPot, Scalar(0, 255, 0), 2, 8, 0);
		cv::rectangle(src, botPot, Scalar(0, 0, 255), 2, 8, 0);
		cv::rectangle(src, playerHand, Scalar(255, 255, 0), 2, 8, 0);
		cv::rectangle(src, playerPot, Scalar(255, 0, 0), 2, 8, 0);
		cv::rectangle(src, board, Scalar(0, 0, 0), 2, 8, 0);
		cv::rectangle(src, indareaparam, Scalar(255, 0, 0), 2, 8, 0);

		// Main footage of the poker game
		cv::imshow("Poker Bot", src);

		if (skipframes > 0) {
			skipframes--;
			//cout << "buckteeth" << endl;
		}
		else {
			
			indarea = src(indareaparam);
			if (CardMotion(indarea, indareaprev)) {
				cout << "TESTING TESTING TESTING" << endl;
				nextstate = true;
				skipframes = 60;		//assuming 60fps camera this will skip the next 2 seconds of action
			}
		}

		if (nextstate) {

			nextstate = false;

			Mat currpot = src(mainPot);
			curropcards = src(playerHand);
			compchipsprev = src(botPot);
			oppchipsprev = src(playerPot);


			if (state.compare("w4h") == 0) {	//waiting for next round
				string cardValue;
				cout << "@waitforhand" << endl;

				if (reset) {
					cardsrevealed = 0;
					hara = "";
					betting = false;

					for (int i = 0; i < 13; i++) {
						for (int j = 0; j < 4; j++) {
							rating[i][j] = 0;
						}
					}

					pc1 = "";
					pc2 = "";
					hara = "";
					cardsdealt = false;
					potval = 0;

					//scan computer chips, opponent chips and pot here


					reset = false;
				}



				//loop card parsing here
				cardValue = detection(src);
				if (pc1.compare("") == 0)
				{
					pc1 = cardValue;
				}
				else if (pc2.compare("") == 0)
				{
					pc2 = cardValue;
				}
				//check if pc1 has a value, if not assign card1, update card matrix

				//if above check failed assign value to pc2, update card matrix, change "state" to "inibet"

				if (pc1.compare("") != 0 && pc2.compare("") != 0) {
					state = "inibet";
					prevopcards = src(playerHand);
				}

			}
			else if (state.compare("inibet") == 0) {

				cout << "@inibet" << endl;

				//scan pot size
				regCardRating(pc1, rating);
				regCardRating(pc2, rating);

				compchips = calcChipValue(compchipsprev);
				oppchips = calcChipValue(oppchipsprev);
				potval = calcChipValue(potprev);

				//guess everyone's chip counts here
				//check facial expressions
				//calc initial chances of victory based on just hand
				//make bet / check / raise / fold

				if (CardMotion(curropcards, prevopcards)) {
					std::cout << "opponent folded" << endl;
					state = "dst";
				}
				else {
					if (!betting) {
						//find hand value
						int c1v = stoi(pc1.substr(1));
						int c2v = stoi(pc2.substr(1));
						String c1s = pc1.substr(0, 1);
						String c2s = pc2.substr(0, 1);

						if (c1v == c2v) {
							comphigh = c1v;
							handstrength = c1v;

							hara = "p" + comphigh;
							cout << "pair of " + c1v << endl;
						}
						else {
							if (c1v > c2v) {
								comphigh = c1v;
								handstrength = c1v * 0.1;
							}
							else {
								comphigh = c2v;
								handstrength = c2v * 0.1;
							}

							hara = "a" + comphigh;
							cout << "high card " + comphigh << endl;
						}

						//augment handstrength by computer's money relative to opponent's money, higher ratio = more risks = lower threshold

						//initial fold threshold
						thresh = 0.45;	//high card 6's handstrength
						if (oppchips != 0)
						{
							if ((compchips / oppchips) > 2) {
								thresh = thresh / 2;
							}
							else if ((compchips / oppchips) < 0.5) {
								thresh = thresh * 2;
							}
						}
						if (handstrength <= thresh) {
							cout << "fold" << endl;
							state = "dst";
						}
						else {

							betting = true;
							//find pot value
							//feed in newest frame cut by bounding box for pot
							//potval = calcChipValue(newestframe, boundingboxforpot);
							if (handstrength > 1) {
								if (compchips >= 1000) {
									cout << "bet 200" << endl;
									potval += 200;
									mylastbet = 200;
								}
								else {
									cout << "bet 100" << endl;
									potval += 100;
									mylastbet = 100;
								}
							}
							else {
								if (compchips >= 1000) {
									cout << "bet 100" << endl;
									potval += 100;
									mylastbet = 100;
								}
								else {
									cout << "bet 50" << endl;
									potval += 50;
									mylastbet = 50;
								}
							}

						}
					}
					else {	//in betting cycle

						//draw region of interest for current pot -> Mat currpot
						
						int npv = calcChipValue(currpot);
						if (npv - potval > 0) {						//computer goes first so pot must change or opponent folded
							int opponentbet = npv - potval;
							if (opponentbet - mylastbet > 10) {		//assume opponent raised
								if (compchips > opponentbet) {
									thresh = thresh * 1.3;
									if (handstrength < thresh) {
										cout << "fold" << endl;
									}
									else {
										cout << "check" << endl;
										mylastbet += opponentbet - mylastbet;
									}
								}
								else {
									cout << "all in" << endl;
									callin = true;
									state = "flop";
									betting = false;
								}

							}
							else {								//assume opponent checked
								cout << "check" << endl;
								state = "flop";
								betting = false;
							}
						}
						else {			//assume opponent did not have chips to match, they went all in. if opponent folded would have been caught earlier in the flow
							pallin = true;
							betting = false;
							state = "flop";
						}
					}


				}
			}
			//
			//
			//redo logic for all states below this comment
			//
			//
			else if (state.compare("flop") == 0) {

				cout << "@flop" << endl;

				//perform actions if did not fold
				//scan pot size
				//get three card values
				//add to rankings 2darray
				//update chances

				potval = calcChipValue(potprev);


				//second round of betting

				if (!betting) {		//waiting for cards to be revealed
					//parse current card here
					cardsrevealed++;

					regCardRating(detection(src), rating);


					//all cards dealt, calc new handstrength, move to betting phase
					if (cardsrevealed == 3) {
						calcHand();

						thresh = thresh * 1.5;

						if (handstrength < thresh) {
							cout << "check" << endl;
						}
						else {
							if (compchips >(mylastbet * 1.5)) {
								cout << "raise " + std::to_string(((mylastbet*1.5) + 2.5) / 5 * 5);
							}
							else {
								int remaining = compchips - mylastbet;
								cout << "raise " + std::to_string(((remaining / 2) + 2.5) / 5 * 5);
							}
						}
						betting = true;
					}
				}
				else {			//betting after flop revealed
					if (callin || pallin) {
						betting = false;
						state = "dst";
					}
					//do betting here
					//prob same as in inibet phase
					else {


						//scan for current pot

						int npv = calcChipValue(currpot);
						if (npv - potval > 0) {						//computer goes first so pot must change or opponent folded
							int opponentbet = npv - potval;
							if (opponentbet - mylastbet > 10) {		//assume opponent raised
								if (compchips > opponentbet) {
									thresh = thresh * 1.3;
									if (handstrength < thresh) {
										cout << "fold" << endl;
									}
									else {
										cout << "check" << endl;
										mylastbet += opponentbet - mylastbet;
									}
								}
								else {
									cout << "all in" << endl;
									callin = true;
									state = "turn";
									betting = false;
								}

							}
							else {								//assume opponent checked
								cout << "check" << endl;
								state = "turn";
								betting = false;
							}
						}
						else {			//assume opponent did not have chips to match, they went all in. if opponent folded would have been caught earlier in the flow
							pallin = true;
							betting = false;
							state = "turn";
						}



					}

				}

			}
			else if (state.compare("turn") == 0) {

				cout << "@turn" << endl;

				//perform actions if did not fold
				//scan pot size
				//get turn card
				//update ranking 2darray
				//update chances

				//third round of betting

				if (!betting) {
					//get and parse card
					regCardRating(detection(src), rating);
					cardsrevealed++;
					calcHand();

					thresh = thresh * 1.2;

					if (handstrength < thresh) {
						cout << "check" << endl;
					}
					else {
						if (compchips >(mylastbet * 1.5)) {
							cout << "raise " + std::to_string(((mylastbet*1.3) + 2.5) / 5 * 5);
						}
						else {
							int remaining = compchips - mylastbet;
							cout << "raise " + std::to_string(((remaining / 3) + 2.5) / 5 * 5);
						}
					}
					betting = true;
				}
				else {
					if (callin || pallin) {
						betting = false;
						state = "dst";
					}
					//betting, same as inibet?
					else {



						//scan for current pot

						int npv = calcChipValue(currpot);
						if (npv - potval > 0) {						//computer goes first so pot must change or opponent folded
							int opponentbet = npv - potval;
							if (opponentbet - mylastbet > 10) {		//assume opponent raised
								if (compchips > opponentbet) {
									thresh = thresh * 1.3;
									if (handstrength < thresh) {
										cout << "fold" << endl;
									}
									else {
										cout << "check" << endl;
										mylastbet += opponentbet - mylastbet;
									}
								}
								else {
									cout << "all in" << endl;
									callin = true;
									state = "river";
									betting = false;
								}

							}
							else {								//assume opponent checked
								cout << "check" << endl;
								state = "river";
								betting = false;
							}
						}
						else {			//assume opponent did not have chips to match, they went all in. if opponent folded would have been caught earlier in the flow
							pallin = true;
							betting = false;
							state = "river";
						}


					}

				}

			}
			else if (state.compare("river") == 0) {

				cout << "@river" << endl;

				//perform actions if did not fold
				//scan pot size
				//get last card
				//update 2darray
				//update chances

				if (!betting) {
					//get abd parse card here
					regCardRating(detection(src), rating);
					cardsrevealed++;
					calcHand();

					thresh = thresh * 1.2;

					if (handstrength < thresh) {
						cout << "check" << endl;
					}
					else {
						if (compchips >(mylastbet * 1.5)) {
							cout << "raise " + std::to_string(((mylastbet*1.3) + 2.5) / 5 * 5);
						}
						else {
							int remaining = compchips - mylastbet;
							cout << "raise " + std::to_string(((remaining / 3) + 2.5) / 5 * 5);
						}
					}
					betting = true;
				}
				else {
					if (callin || pallin) {
						betting = false;
						state = "dst";
					}
					//last round of betting
					else {


						//scan for current pot

						int npv = calcChipValue(currpot);
						if (npv - potval > 0) {						//computer goes first so pot must change or opponent folded
							int opponentbet = npv - potval;
							if (opponentbet - mylastbet > 10) {		//assume opponent raised
								if (compchips > opponentbet) {
									thresh = thresh * 1.3;
									if (handstrength < thresh) {
										cout << "fold" << endl;
									}
									else {
										cout << "check" << endl;
										mylastbet += opponentbet - mylastbet;
									}
								}
								else {
									cout << "all in" << endl;
									callin = true;
									state = "dst";
									betting = false;
								}

							}
							else {								//assume opponent checked
								cout << "check" << endl;
								state = "dst";
								betting = false;
							}
						}
						else {			//assume opponent did not have chips to match, they went all in. if opponent folded would have been caught earlier in the flow
							pallin = true;
							betting = false;
							state = "dst";
						}


					}

				}

			}
			else if (state.compare("dst") == 0) {
				//wait for dealer to prepare next round
				//so for now just skip some frames?
				state = "w4h";
				betting = false;
				reset = true;
				thresh = 0;
				cardsdealt = false;
				mylastbet = 0;
				cout << "----------***----------" << endl;
				Sleep(5000);
			}
		}





		tomcat = src.clone();	//update prev frame
		indareaprev = indarea.clone();


		if (waitKey(30) == 27)
		{
			cout << "esc key is pressed by user" << endl;
			break;
		}

	}

	// Wait until keypress
	waitKey(0);
	return(0);
}

string detection(Mat& src)
{
	Mat src_gray;
	vector<vector<Point>> contours;
	vector<Vec4i> hierarchy;
	string type;

	// Gaussian blur and thresholding
	cvtColor(src, src_gray, CV_BGR2GRAY);
	GaussianBlur(src_gray, src_gray, Size(1, 1), 1000);
	threshold(src_gray, src_gray, 200, 255, THRESH_BINARY | THRESH_OTSU);

	/*imshow("test", src_gray);*/

	// Find contours
	findContours(src_gray, contours, hierarchy, CV_RETR_TREE, CV_CHAIN_APPROX_SIMPLE, Point(0, 0));
	Mat contour_output = Mat::zeros(src_gray.size(), CV_8UC3);

	// Find largest contour
	int maxsize = 0;
	int maxind = 0;
	Rect boundrec;

	for (int i = 0; i < 52; i++)
	{
		if (contours.size() > 0){
			for (int j = 0; j < contours.size(); j++)
			{
				// Documentation on contourArea: http://docs.opencv.org/modules/imgproc/doc/structural_analysis_and_shape_descriptors.html#
				double area = contourArea(contours[j]);
				if (area > maxsize) {
					maxsize = area;
					maxind = j;
					boundrec = boundingRect(contours[j]);
				}
			}

			// Draw contours
			// Documentation for drawing contours: http://docs.opencv.org/modules/imgproc/doc/structural_analysis_and_shape_descriptors.html?highlight=drawcontours#drawcontours
			drawContours(contour_output, contours, maxind, Scalar(255, 0, 0), CV_FILLED, 8, hierarchy);
			drawContours(contour_output, contours, maxind, Scalar(0, 0, 255), 2, 8, hierarchy);
			// Documentation for drawing rectangle: http://docs.opencv.org/modules/core/doc/drawing_functions.html
			cv::rectangle(contour_output, boundrec, Scalar(0, 255, 0), 1, 8, 0);

			string suiteValue = checkCardSuite(src, boundrec, i % 4);
			string numValue = checkCardNumber(src, boundrec, i % 13);
			type = suiteValue + numValue;
		}
	}

	
	return type;
}

string checkCardSuite(Mat& binaryimg, Rect& boundary, int i){
	Mat cardimg; Mat templ; Mat result; Mat scaled;
	int match_method = 5;
	i %= 4;

	// Crop image
	cardimg = binaryimg(boundary);

	Rect readZone = Rect(0, 0, 200, 280);
	/*rectangle(binaryimg, readZone, Scalar(0, 0, 255), 2, 8, 0);*/
	Mat readArea = binaryimg(readZone);

	
	// Determine which template to compare against
	templ = retrieveSuiteTemplates(i);

	// Grayscale template as well as reading area
	cvtColor(readArea, readArea, CV_BGR2GRAY);

	double matchSuiteValue = match(readArea, templ);	
	if (matchSuiteValue < minSuiteMatrix)
	{
		minSuiteIndex = i;
		minSuiteMatrix = matchSuiteValue;
	}
	switch (minSuiteIndex)
	{
	case 0: return "c"; break;
	case 1: return "d"; break;
	case 2: return "h"; break;
	case 3: return "s"; break;
	default:
		return to_string(int(rand() % 4 + 1));
	}
}

string checkCardNumber(Mat& binaryimg, Rect& boundary, int i){
	Mat cardimg; Mat templ; Mat result; Mat scaled;
	int match_method = 5;

	// Crop image
	cardimg = binaryimg(boundary);

	Rect readZone = Rect(0, 0, 200, 280);
	Mat readArea = binaryimg(readZone);

	// Determine which template to compare against
	templ = retrieveNumberTemplates(i);

	// Grayscale template as well as reading area
	cvtColor(readArea, readArea, CV_BGR2GRAY);
	double matchNumValue = match(readArea, templ);
	if (matchNumValue < minNumMatrix)
	{
		minNumIndex = i;
		minNumMatrix = matchNumValue;
	}
	switch (minNumIndex)
	{
	case 0: return "1"; break;
	case 1: return "2"; break;
	case 2: return "3"; break;
	case 3: return "4"; break;
	case 4: return "5"; break;
	case 5: return "6"; break;
	case 6: return "7"; break;
	case 7: return "8"; break;
	case 8: return "9"; break;
	case 9: return "10"; break;
	case 10: return "11"; break;
	case 11: return "12"; break;
	case 12: return "13"; break;
	default:
		return to_string(int(rand() % 13 + 1));
	}
}

Mat process(Mat& img)
{
	Mat imggray; Mat blur; Mat thresh; Mat blur_thresh;
	GaussianBlur(img, blur, Size(5, 5), 2);
	adaptiveThreshold(blur, thresh, 255, 1, 1, 11, 1);
	GaussianBlur(thresh, blur_thresh, Size(5, 5), 5);
	addWeighted(blur_thresh,1.5, blur_thresh, -0.5, 0, blur_thresh);
	return blur_thresh;
}

double match(Mat& img, Mat& templ)
{
	Mat zone_display; Mat result;
	img.copyTo(zone_display);

	int result_cols = img.cols - templ.cols + 1;
	int result_rows = img.rows - templ.rows + 1;

	result.create(result_rows, result_cols, CV_32FC1);

	matchTemplate(img, templ, result, 5);
	normalize(result, result, 0, 1, NORM_MINMAX, -1, Mat());

	double minVal; double maxVal; Point minLoc; Point maxLoc;
	Point matchLoc;

	minMaxLoc(result, &minVal, &maxVal, &minLoc, &maxLoc, Mat());

	matchLoc = maxLoc;

	// Draw rectangle around the detected symbol
	cv::rectangle(zone_display, matchLoc, Point(matchLoc.x + templ.cols, matchLoc.y + templ.rows), Scalar(0,0,255), 2, 8, 0);
	/*cv::imshow("Template zone", zone_display);*/

	// Crop out the detected symbol for beter analysis
	Rect symbol = Rect(matchLoc.x, matchLoc.y, templ.cols, templ.rows);
	Mat matcharea = zone_display(symbol);
	
	Mat diff;
	absdiff(process(templ), process(matcharea), diff);
	GaussianBlur(diff, diff, Size(5, 5), 5);
	threshold(diff, diff, 200, 255, CV_THRESH_BINARY);
	/*imshow("Test diff", diff);*/
	Scalar value = sum(diff);
	return (value[0]+value[1]+value[2]);
}

/// Retrieve templates for playing cards
Mat retrieveSuiteTemplates(int ref){
	Mat temp;
	
	if(ref == 0)
		temp = imread("templates/c.jpg",0);
	else if(ref == 1)
		temp = imread("templates/d.jpg",0);
	else if(ref == 2)
		temp = imread("templates/h.jpg",0);
	else if (ref == 3)
		temp = imread("templates/s.jpg",0);
	else
		cout << "ref not valid" << endl;
	if (temp.empty())
		cout << "temp empty" << endl;
	return temp;
}

Mat retrieveNumberTemplates(int ref){
	Mat temp;
	if (ref == 0)
		temp = imread("templates/a.jpg",0); 
	else if (ref == 1)
		temp = imread("templates/2.jpg",0); 
	else if (ref == 2)
		temp = imread("templates/3.jpg",0); 
	else if (ref == 3)
		temp = imread("templates/4.jpg",0); 
	else if (ref == 4)
		temp = imread("templates/5.jpg",0); 
	else if (ref == 5)
		temp = imread("templates/6.jpg",0); 
	else if (ref == 6)
		temp = imread("templates/7.jpg",0); 
	else if (ref == 7)
		temp = imread("templates/8.jpg",0); 
	else if (ref == 8)
		temp = imread("templates/9.jpg",0); 
	else if (ref == 9)
		temp = imread("templates/10.jpg",0); 
	else if (ref == 10)
		temp = imread("templates/j.jpg",0); 
	else if (ref == 11)
		temp = imread("templates/q.jpg",0); 
	else if (ref == 12)
		temp = imread("templates/k.jpg",0);
	else
		cout << "ref out of bounds" << endl;
	if (temp.empty()){
		cout << "empty temp" << endl;
	}
	//imshow("Number Template", temp);
	return temp;
}

/** @function detectAndDisplay */
void detectAndDisplay(Mat frame)
{
	std::vector<Rect> faces;
	Mat frame_gray;

	cvtColor(frame, frame_gray, COLOR_BGR2GRAY);
	equalizeHist(frame_gray, frame_gray);

	//-- Detect faces
	face_cascade.detectMultiScale(frame_gray, faces, 1.1, 2, 0 | CASCADE_SCALE_IMAGE, Size(30, 30));
	/*
	scaleFactor – Parameter specifying how much the image size is reduced at each image scale.

	Basically the scale factor is used to create your scale pyramid.More explanation can be found here.In short, as described here, your model has a fixed size defined during training, which is visible in the xml.This means that this size of face is detected in the image if present.However, by rescaling the input image, you can resize a larger face to a smaller one, making it detectable by the algorithm.

	1.05 is a good possible value for this, which means you use a small step for resizing, i.e.reduce size by 5 % , you increase the chance of a matching size with the model for detection is found.This also means that the algorithm works slower since it is more thorough.You may increase it to as much as 1.4 for faster detection, with the risk of missing some faces altogether.
	minNeighbors – Parameter specifying how many neighbors each candidate rectangle should have to retain it.

	This parameter will affect the quality of the detected faces.Higher value results in less detections but with higher quality. 3~6 is a good value for it.
	minSize – Minimum possible object size.Objects smaller than that are ignored.

	This parameter determine how small size you want to detect.You decide it!Usually, [30, 30] is a good start for face detection.
	maxSize – Maximum possible object size.Objects bigger than this are ignored.

	This parameter determine how big size you want to detect.Again, you decide it!Usually, you don't need to set it manually, the default value assumes you want to detect without an upper limit on the size of the face.*/




	for (size_t i = 0; i < faces.size(); i++)
	{
		Point center(faces[i].x + faces[i].width / 2, faces[i].y + faces[i].height / 2);
		ellipse(frame, center, Size(faces[i].width / 2, faces[i].height / 2), 0, 0, 360, Scalar(255, 0, 255), 4, 8, 0);

		Mat faceROI = frame_gray(faces[i]);
		std::vector<Rect> eyes;

		//-- In each face, detect eyes
		eyes_cascade.detectMultiScale(faceROI, eyes, 1.1, 2, 0 | CASCADE_SCALE_IMAGE, Size(30, 30));

		for (size_t j = 0; j < eyes.size(); j++)
		{
			Point eye_center(faces[i].x + eyes[j].x + eyes[j].width / 2, faces[i].y + eyes[j].y + eyes[j].height / 2);
			int radius = cvRound((eyes[j].width + eyes[j].height)*0.25);
			circle(frame, eye_center, radius, Scalar(255, 0, 0), 4, 8, 0);
		}
	}
	//-- Show what you got
	cv::imshow(window_name, frame);
}

bool CardMotion(Mat& curr, Mat& prev) {
	Mat tempres = curr.clone();
	absdiff(prev, curr, tempres);

	int mvthresh = 2000;	//if 200 pixels show change confirm it
	int cmv = 0;
	//check diff map for movement
	for (int i = 0; i < tempres.rows; i++) {
		for (int j = 0; j < tempres.cols; j++) {
			Vec3b dstcol = tempres.at<Vec3b>(Point(j, i));
			if ((dstcol[0] + dstcol[1] + dstcol[2]) > 60) {
				cmv++;
			}
			if (cmv > mvthresh) {
				return true;
			}
		}
	}
	return false;
}

void regCardRating(String card, int rat[13][4]) {
	String suit = card.substr(0, 1);
	int val = stoi(card.substr(1));
	int suitval;

	if (suit.compare("s")) {
		suitval = 0;
	}
	else if (suit.compare("c")) {
		suitval = 1;
	}
	else if (suit.compare("d")) {
		suitval = 2;
	}
	else if (suit.compare("h")) {
		suitval = 3;
	}

	val--;	//val is originally the actual value of the card (1-13) but the array has indexes (0-12)

	rat[val][suitval] = 1;
}

int calcChipValue(Mat& curr) {

	int redpx = 0;
	int bluepx = 0;
	int greenpx = 0;

	int chipsizeinpx = 1000;

	//get approx size of cutouts in px later and use as denominator

	for (int i = 0; i < curr.rows; i++) {
		for (int j = 0; j < curr.cols; j++) {
			Vec3b dstcol = curr.at<Vec3b>(Point(j, i));
			if (dstcol[0] > 175) {		//blue?
				bluepx++;
			}
			else if (dstcol[1] > 175) {		//green?
				greenpx++;
			}
			else if (dstcol[2] > 175) {		//red?
				redpx++;
			}
		}
	}

	int redchips = redpx / chipsizeinpx;
	int bluechips = bluepx / chipsizeinpx;
	int greenchips = greenpx / chipsizeinpx;

	return redchips * 25 + bluechips * 10 + greenchips * 5;
}

void calcHand() {
	int tempsame = 0;
	int numsameval = 0;
	int sets = 0;
	int firstsetnum = 0;
	int secondsetnum = 0;
	int firstsetval = 0;
	int secondsetval = 0;
	bool cardwithval[13];

	//check cards shown matrix to determine best hand
	for (int i = 0; i < 13; i++) {
		for (int j = 0; j < 4; j++) {
			if (::rating[i][j] == 2) {
				numsameval++;
				cardwithval[i] = true;
			}
			else if (rating[i][j] == 1) {
				tempsame++;
				cardwithval[i] = true;
			}
		}
		if (numsameval > 0) {
			int tot = tempsame + 1;
			if (tot > 1) {
				sets++;
			}
			if (sets == 1) {
				firstsetnum = tot;
				firstsetval = i + 2;
			}
			else if (sets == 2) {
				secondsetnum = tot;
				secondsetval = i + 2;
			}
		}
		else {
			cardwithval[i] = false;
		}
		numsameval = 0;
		tempsame = 0;
	}

	//check royal flush
	if (cardwithval[12] && cardwithval[11] && cardwithval[10] && cardwithval[9] && cardwithval[8]) {
		for (int i = 0; i < 4; i++) {
			if ((rating[12][i] + rating[11][i] + rating[10][i] + rating[9][i] + rating[8][i]) > 5) {	//need at least 1 of these to be owned by
				handstrength = 100000000;																//computer hand so sum either 6 or 7
				hara = "r";
				cout << "royal flush" << endl;
			}
		}
	}
	else {
		//check straight flush
		bool sflush = false;
		int fsum = 0;
		for (int j = 0; j < 4; j++) {
			for (int i = 0; i < 8; i++) {
				fsum = rating[i][j] + rating[i + 1][j] + rating[i + 2][j] + rating[i + 3][j] + rating[i + 4][j];
				if (fsum > 5) {
					sflush = true;
					handstrength = 10000000 + ((i + 4) * 1000000);
					hara = "u" + (i + 4);
				}
			}
		}
		if (sflush) {
			return;
		}

		//check 4 of a kind
		if (firstsetnum == 4) {
			hara = "f" + firstsetval;
			handstrength = 1000000 + (firstsetval * 100000);
			return;
		}
		else if (secondsetnum == 4) {
			hara = "f" + secondsetval;
			handstrength = 1000000 + (secondsetval * 100000);
			return;
		}

		//check full house
		if (firstsetnum == 2 && secondsetnum == 3) {
			hara = "h" + std::to_string(secondsetval) + "/" + std::to_string(firstsetval);
			handstrength = 100000 + secondsetval * 50000 + 5000 * firstsetval;
			return;
		}
		else if (firstsetnum == 3 && secondsetnum == 2) {
			hara = "h" + std::to_string(firstsetval) + "/" + std::to_string(secondsetval);
			handstrength = 100000 + firstsetval * 50000 + 5000 * secondsetval;
			return;
		}

		//check flush
		bool flush = false;
		for (int j = 0; j < 4; j++) {
			fsum = 0;
			for (int i = 0; i < 13; i++) {
				fsum = fsum + rating[i][j];
			}
			if (fsum > 5) {
				flush = true;
				handstrength = 25000;
				hara = "w0";
			}
		}
		if (flush) {
			return;
		}

		//check straight
		bool straight = false;
		for (int i = 0; i < 9; i++) {
			if (cardwithval[i] && cardwithval[i + 1] && cardwithval[i + 2] && cardwithval[i + 3] && cardwithval[i + 4]) {
				straight = true;
				handstrength = 1000 * (i + 4);
				hara = "s" + (i + 4);
			}
		}
		if (straight) {
			return;
		}

		//3 of a kind
		if (firstsetnum == 3) {
			hara = "t" + firstsetval;
			handstrength = 100 + firstsetval * 5;
			return;
		}
		else if (secondsetnum == 3){
			hara = "t" + secondsetval;
			handstrength = 100 + secondsetval * 5;
			return;
		}

		//two pairs
		if (firstsetnum == 2 && secondsetnum == 2) {
			hara = "i" + std::to_string(firstsetval) + "," + std::to_string(secondsetval);
			handstrength = 10 + 3 * (firstsetval + secondsetval);
			return;
		}

		//pair
		if (firstsetnum == 2) {
			hara = "p" + firstsetval;
			handstrength = firstsetval;
			return;
		}
		else if (secondsetnum == 2) {
			hara = "p" + secondsetval;
			handstrength = secondsetval;
			return;
		}

		//high card;
		//shouldn't have to do anything as in this case hand should already be in high card state
	}
}