#include "CMT.h"

//#define DEBUG_MODE

#ifdef DEBUG_MODE
#	include <QDebug>
#endif


class UserSelections
{
public:

	cv::Point2d p0;
	cv::Point2d p1;
	std::deque<cv::Rect> rects;

	UserSelections()
	{
		p0 = p1 = cv::Point2d(-1,-1);
	}

	void draw(cv::Mat& img)
	{
		if (img.empty())
			return;
		if (! rects.empty())
			for (unsigned i = 0; i < rects.size(); ++i)
				cv::rectangle(img, rects[i], CV_RGB(0,0,0));
		if (p0 != cv::Point2d(-1,-1) && p1 != cv::Point2d(-1,-1) && p0 != p1)
			cv::rectangle(img, p0, p1, CV_RGB(128,128,128));
	}
};


void mouse_callback_function(int event, int x, int y, int flags, void* userdata)
{
	UserSelections* ud = NULL;
	if (userdata != NULL)
		ud = static_cast<UserSelections*>(userdata);

	if (event == cv::EVENT_MOUSEMOVE)
	{
		if (ud && ud->p0 != cv::Point2d(-1,-1))
			ud->p1 = cv::Point2d(x,y);
	}
	else if (event == cv::EVENT_LBUTTONDOWN)
	{
		std::cout << "Left mouse button down - position (" << x << ", " << y << ")" << std::endl;
		if (ud)
			ud->p0 = ud->p1 = cv::Point2d(x, y);
	}
	else if (event == cv::EVENT_LBUTTONUP)
	{
		std::cout << "Left mouse button up - position (" << x << ", " << y << ")" << std::endl;
		if (ud && ud->p0 != cv::Point2d(-1,-1))
		{
			cv::Rect rect = cv::Rect(ud->p0, cv::Point(x, y));
			if (rect.area() > 0)
			{
				ud->rects.push_back(rect);
				std::cout << ud->rects[ud->rects.size()-1] << std::endl;
			}
			ud->p0 = ud->p1 = cv::Point2d(-1,-1);
		}
	}
}


void num2str(char *str, int length, int num)
{
    for(int i = 0; i < length-1; i++)
    {
        str[length-i-2] = '0'+num%10;
        num /= 10;
    }
    str[length-1] = 0;
}


int main(int argc, char *argv[])
{
	try
	{
		// command-line parameters
		const char* keys =
		  "{h    |help                  |false               |Print this help message.}"
		  "{v    |video                 |                    |Specify the video file.}"
		  "{p    |image_path            |                    |Specify the path of the image files.}"
		  "{nl   |image_num_length      |8                   |Specify the length of the numeric part of the image filename.}"
		  "{ext  |image_extension       |png                 |The image file extension.}"
		  "{s    |image_start           |1                   |Image start number.}"
		  "{e    |image_end             |10000               |Image end number.}"
		  "{tlx  |box_top_left_x        |-1                  |x-coord of top left corner of the bounding box.}"
		  "{tly  |box_top_left_y        |-1                  |y-coord of top left corner of the bounding box.}"
		  "{brx  |box_bottom_right_x    |-1                  |x-coord of bottom right corner of the bounding box.}"
		  "{bry  |box_bottom_right_y    |-1                  |y-coord of bottom right corner of the bounding box.}"
		  "{dt   |detector_type         |Feature2D.BRISK     |The detector type.}"
		  "{dst  |descriptor_type       |Feature2D.BRISK     |The descriptor type.}"
		  "{mt   |matcher_type          |BruteForce-Hamming  |The matcher type.}"
		  "{to   |threshold_outlier     |20                  |Outlier threshold.}"
		  "{tc   |threshold_confidence  |0.75                |threshold confidence.}"
		  "{tr   |threshold_ratio       |0.8                 |Threhsold Ratio.}"
		  "{dl   |descriptor_length     |512                 |Descriptor Length.}"
		  "{er   |estimate_rotation     |true                |Estimate Rotation?}"
		  "{es   |estimate_scale        |true                |Estimate Scale?}"
		  ;
		cv::CommandLineParser cmd(argc, argv, keys);

		if (argc <= 1 || cmd.get<bool>("help") == true)
		{
		  std::cout << "Usage: " << argv[0] << " [options]" << std::endl << "Available options:" << std::endl;
		  cmd.printParams();
		  return 1;
		}


		// prepare the input source: video or image list
		std::string video_path = cmd.get<std::string>("video");
		std::string image_ext, image_path = cmd.get<std::string>("image_path");
		int image_num_length = 8, image_start = 1, image_end = 10000;
		cv::VideoCapture vf;

		if(video_path.empty() && image_path.empty())
		{
			std::cout << "!! No input source specified. Must be one of: a video or an image list." << std::endl;
			return 1;
		}


		// the video file
		if (! video_path.empty())
		{
			std::cout << "Processing video file [" << video_path << "]" << std::endl << std::endl;

			bool ok = vf.open(video_path);
			if(! vf.isOpened())
			{
				std::cout << "!! Error opening video file" << std::endl;
				return 1;
			}
		}
		else   // image list
		{
			image_ext = cmd.get<std::string>("image_extension");
			image_num_length = cmd.get<int>("image_num_length");
			image_start = cmd.get<int>("image_start");
			image_end = cmd.get<int>("image_end");

			char s1[100], s2[100];        
			num2str(s1, image_num_length+1, image_start);
			num2str(s2, image_num_length+1, image_end);
			std::cout << "Processing image list, from: [" << image_path << s1 << "." << image_ext << "]" << std::endl 
					  << "                         to: [" << image_path << s2 << "." << image_ext << "]" << std::endl << std::endl;
		}


		// CMT initialisation
		CMT cmt;
		cmt.detectorType = cmd.get<std::string>("detector_type");
		cmt.descriptorType = cmd.get<std::string>("descriptor_type");
		cmt.matcherType = cmd.get<std::string>("matcher_type");
		cmt.thrOutlier = cmd.get<int>("threshold_outlier");
		cmt.thrConf = cmd.get<float>("threshold_confidence");
		cmt.thrRatio = cmd.get<float>("threshold_ratio");
		cmt.descriptorLength = cmd.get<int>("descriptor_length");
		cmt.estimateRotation = cmd.get<bool>("estimate_rotation");
		cmt.estimateScale = cmd.get<bool>("estimate_scale");


		cv::namedWindow("frame", cv::WINDOW_NORMAL);

		// functionality to allow user selection of bounding box
		UserSelections user_selections;
		cv::setMouseCallback("frame", mouse_callback_function, &user_selections);	


		// program state variables
		int program_state = 1;		// 1 = object-selection stage, 2 = object tracking mode
		int prev_program_state = 0;
		cv::Mat img, im_gray, im_out;

		
		// bounding box specified on the command line?
		cv::Point2f initTopLeft(cmd.get<int>("box_top_left_x"), cmd.get<int>("box_top_left_y"));
		cv::Point2f initBottomDown(cmd.get<int>("box_bottom_right_x"), cmd.get<int>("box_bottom_right_y"));
		if (initTopLeft != cv::Point2f(-1,-1) && initBottomDown != cv::Point2f(-1,-1))
		{
			user_selections.rects.push_back(cv::Rect(initTopLeft, initBottomDown));
			program_state = 2;
			prev_program_state = 1;
		}


		std::cout << "** Press <Q> or <q> or <ESC> to quit the program." << std::endl;
		std::cout << "** Press <S> or <s> to select the object to be tracked." << std::endl;
		std::cout << "** Press <T> or <t> to track the object." << std::endl;
		std::cout << "** Press <R> or <r> to restart the video/image sequence." << std::endl << std::endl;

		long frame_num = (video_path.empty() ? image_start : 1);
		while(frame_num <= image_end || vf.isOpened())
		{
			// load an image
			if (program_state == 2 || (program_state == 1 && img.empty()))
			{
				if (vf.isOpened())
				{
					vf >> img;				// video capture
				}
				else
				{
					char filename[2048];    
					char numString[100];
        
					num2str(numString, image_num_length+1, frame_num);
					sprintf(filename, "%s%s.%s", image_path.c_str(), numString, image_ext.c_str());

					#ifdef DEBUG_MODE
					   qDebug() << filename;
					#endif

					img = cv::imread(filename);
					if (img.empty())
						std::cout << "Cannot open image file: [" << filename << "]" << std::endl;
				}

				if (img.empty())
				{
					std::cout << "!! Probable EOS reached" << std::endl;
					break;
				}

				cv::cvtColor(img, im_gray, CV_RGB2GRAY);
				++frame_num;
			}

			im_out = img.clone();

			if (program_state == 1 && prev_program_state != 1)				// // transitioning to state 1
			{
				user_selections.rects.clear();

				std::cout << "Use the mouse left button to select the top-left corner of the object's bounding box and then drag to the bottom-right corner and release. Press <T> or <t> to enter the object tracking mode." << std::endl << std::endl;
			}

			if (program_state == 1)
			{
				user_selections.draw(im_out);
			}

			// model initialisation
			if (prev_program_state == 1 && program_state == 2)				// transitioning to state 2
			{
				if (user_selections.rects.empty())
					throw std::exception("No object has been selected!!");

				cv::Rect r = user_selections.rects.back();
				initTopLeft = r.tl();
				initBottomDown = r.br();

				std::cout << "learning model for bbox: " << initTopLeft << " .. " << initBottomDown << std::endl;
				cmt.initialise(im_gray, initTopLeft, initBottomDown);

				std::cout << "tracking... (press Q to quit)..." << std::endl;
			}

			if (program_state == 2)
			{
				// tracking
				cmt.processFrame(im_gray);

				// draw the keypoints
				for(int i = 0; i<cmt.trackedKeypoints.size(); i++)
					cv::circle(im_out, cmt.trackedKeypoints[i].first.pt, 3, cv::Scalar(255,255,255));

				// bounding box
				cv::line(im_out, cmt.topLeft, cmt.topRight, cv::Scalar(255,255,255));
				cv::line(im_out, cmt.topRight, cmt.bottomRight, cv::Scalar(255,255,255));
				cv::line(im_out, cmt.bottomRight, cmt.bottomLeft, cv::Scalar(255,255,255));
				cv::line(im_out, cmt.bottomLeft, cmt.topLeft, cv::Scalar(255,255,255));

				#ifdef DEBUG_MODE
					qDebug() << "trackedKeypoints";
					for(int i = 0; i<cmt.trackedKeypoints.size(); i++)
						qDebug() << cmt.trackedKeypoints[i].first.pt.x << cmt.trackedKeypoints[i].first.pt.x << cmt.trackedKeypoints[i].second;
					qDebug() << "box";
					qDebug() << cmt.topLeft.x << cmt.topLeft.y;
					qDebug() << cmt.topRight.x << cmt.topRight.y;
					qDebug() << cmt.bottomRight.x << cmt.bottomRight.y;
					qDebug() << cmt.bottomLeft.x << cmt.bottomLeft.y;
				#endif
			}

			imshow("frame", im_out);
			int ky = cv::waitKey(1);

			prev_program_state = program_state;

			if (ky == 'Q' || ky == 'q' || ky == 27)
			{
				std::cout << "!! User aborted the video acquisition" << std::endl;
				break;
			}
			else if (ky == 'T' || ky == 't')
			{
				program_state = 2;
			}
			else if (ky == 'S' || ky == 's')
			{
				program_state = 1;
			}
			else if (ky == 'R' || ky == 'r')
			{
				frame_num = (video_path.empty() ? image_start : 1);
				if (vf.isOpened())
					vf.set(CV_CAP_PROP_POS_FRAMES, 0);
			}
		}

		std::cout << "Ready" << std::endl;
	}
	catch(std::exception& x)
	{
		std::cout << "!!EXCEPTION!! " << x.what() << std::endl;
	}
	catch(...)
	{
		std::cout << "!!EXCEPTION!!" << std::endl;
	}

    return 0;
}
