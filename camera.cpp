#include "camera.h"

#include <QFile>
#include <QTextStream>

#include "src_contours.hpp"
#include <sys/stat.h>

cv::Size Camera::Template::posterSize(0, 0);

Camera::Camera()
{

}

Camera::Camera(const string &calibFilePath, int index_, float sf_,
               float roi_, int cntrMinSize_) :
    index(index_),
    sf(sf_),
    roi(roi_),
    cntrMinSize(cntrMinSize_)
{
    if(model.loadModel(calibFilePath)) {
        qInfo() << "load model failed";
        exit(-1);
    }
    model.createLUT(xmap, ymap, sf);
}

int Camera::setIntrinsic(const string &path, const string &name, int img_num, cv::Size patternSize)
{
    /***************************************** 1.Load chessboard image ****************************************/
    std::vector<std::vector<cv::Point3f> > object_points;
    std::vector<std::vector<cv::Point2f> > image_points;

    // Define chessboard corners in 3D spase
    std::vector<cv::Point3f> obj;		// Chessboard corners in 3D spase
    for(int i = 0; i < patternSize.height; ++i)
        for(int j = 0; j < patternSize.width; ++j)
            obj.push_back(cv::Point3f(j, i, 0.0f));

    for (int i = 0; i < img_num; i++)
    {
        char img_name[path.length() + name.length() + 7];
        sprintf(img_name, "%s%s%d.jpg", path.c_str(),
                name.c_str(), i);
        cv::Mat chessboard_img = cv::imread(img_name, CV_LOAD_IMAGE_COLOR);
        if (chessboard_img.empty()) // Check if chessboard had been loaded
        {
            std::cout << "The " << img_name << " image not found" << std::endl;
            return (-1);
        }

        /************** 2. Calculate maps for fisheye undistortion using Scarramuza calibrating data **************/
        cv::remap(chessboard_img, chessboard_img, xmap, ymap, cv::INTER_LINEAR); //remove fisheye distortion

        /******************************* 3. Calculate camera intrinsic parameters **********************************/
        // Convert the image into a grayscale image
        cv::Mat chessboard_gray;
        cv::cvtColor(chessboard_img, chessboard_gray, CV_BGR2GRAY);

        //Find chessboard corners
        std::vector<cv::Point2f> corners;	// Chessboard corners in 2D camera frame
        if (cv::findChessboardCorners(chessboard_img, patternSize, corners,
                                      CV_CALIB_CB_ADAPTIVE_THRESH | CV_CALIB_CB_FILTER_QUADS))
        {
            // Store points results into the lists
            image_points.push_back(corners);
            object_points.push_back(obj);
        }
    }

    // Calculate intrinsic parameters
    if (object_points.size() > 0)
    {
        param.K = cv::initCameraMatrix2D(object_points, image_points,
                                           model.model.img_size, 0);
        param.distCoeffs= cv::Mat(4, 1, CV_32F, cv::Scalar(0));
        std::cout << "K: \n" << param.K << std::endl;
    }
    else
    {
        std::cout << "Problem with corner detection" << std::endl;
        return (-1);
    }
    return(0);
}

int Camera::setTemplate(const string &tempPath)
{
    QString qpath = QString::fromStdString(tempPath);

    QFile data(qpath);
    if(data.open(QFile::ReadOnly) == false) {
        qInfo() << qpath << " not found";
        return -1;
    }

    int x, y;
    temp.maxX = 0;
    QTextStream in(&data);
    while((in >> x >> y).status() == QTextStream::Ok) {
        temp.ref_points.push_back(cv::Point3f(x, y, 0));
        if(x > temp.maxX)
            temp.maxX = x;
    }

    return 0;
}

void Camera::normTemplate(std::vector<Camera *> &cameras)
{
    float postWidth = cameras.at(0)->temp.maxX;
    float postHeight = postWidth;

    for(uint i = 1; i < cameras.size(); i++) {
        int maxX = cameras.at(i)->temp.maxX;
        if(maxX > postWidth) {
            postWidth = maxX;
            break;
        } else if(maxX < postWidth) {
            postHeight = maxX;
            break;
        }
    }

    for(Camera *cam : cameras) {
        int ny = (cam->temp.maxX == postWidth) ? postHeight : postWidth;

        for(cv::Point3f &refp : cam->temp.ref_points) {
            refp.x = (2 * refp.x - cam->temp.maxX) / postWidth;
            refp.y = (2 * refp.y - ny) / postWidth;
        }
    }
}

int Camera::setExtrinsic(const Mat &img)
{
    using namespace std;
    using namespace cv;

    /******************************************* 1. Defisheye *************************************************/
    Mat und_img;
    remap(img, und_img, xmap, ymap, cv::INTER_LINEAR); //Remap

    /************************************ 3. Get points from distorted image ***********************************/
    img_p.clear();
    if (getImagePoints(und_img, temp.ref_points.size(), img_p) != 0)
        return(-1);
//    for(int i = 0; i < img_p.size(); i++) {
//        cv::putText(und_img, to_string(i), img_p.at(i), cv::FONT_HERSHEY_PLAIN,
//                    1.0, Scalar(0, 0, 233));
//    }
//    cv::imshow("fff", und_img);waitKey(0);

    /************************ 4. Find an object pose from 3D-2D point correspondences. *************************/
    vector<Point2f> image_points;
    vector<Point3f> object_points;
    for (uint j = 0; j < img_p.size(); j++) {
        image_points.push_back(img_p[j]);
        object_points.push_back(Point3f(temp.ref_points[j].x, temp.ref_points[j].y, 0.0));
    }

    Mat matImgPoints(image_points);
    Mat matObjPoints(object_points);
    solvePnP(matObjPoints, matImgPoints, param.K, param.distCoeffs, param.rvec, param.tvec);


#if 0
    /********************************* 5. Calculate camera world position. *************************************
     * Translation and rotation vectors from solvePnP are telling where is the object in camera's coordinates.
     * To determine world coordinates of a camera we need to get an inverse transform.
     ***********************************************************************************************************/
    Mat R;
    Rodrigues(param.rvec, R);
    Mat cameraRotationVector;
    Rodrigues(R.t(), cameraRotationVector);
    Mat cameraTranslationVector = -R.t() * param.tvec;
    cout << "Camera translation  " <<  cameraTranslationVector << endl;
    cout << "Camera rotation  " <<  cameraRotationVector << endl;
#endif

    radius = sqrt((double)pow(temp.ref_points[0].y, 2) + (double)pow(temp.ref_points[0].x, 2));
    return(0);
}

void Camera::updateLUT(float scale)
{
    sf = scale;
    model.createLUT(xmap, ymap, sf);
}

int Camera::getBowlHeight(double radius, double step_x)
{
    if (param.rvec.empty() || param.tvec.empty() || param.K.empty())
        return (0);

    int num = 1;
    bool next_point = true;

    // Get mask for defisheye transformation
    Mat mask(xmap.rows, xmap.cols, CV_8U, Scalar(255));
    remap(mask, mask, xmap, ymap, cv::INTER_LINEAR);

    // Get 3D points projection into 2D image for bowl side with x = 0 (z = (y - radius)^2)
    while((next_point) && (num < 100))
    {
        vector<Point3f> p3d;
        vector<Point2f> p2d;

        double new_point = num * step_x;
        p3d.push_back(Point3f(0, - radius - new_point, - new_point * new_point)); // Get num point for 3D template
        projectPoints(p3d, param.rvec, param.tvec, param.K, param.distCoeffs, p2d); // Project the point into 2D image

        if((p2d[0].y >= 0) && (p2d[0].y < mask.rows) && (p2d[0].x >= 0) && (p2d[0].x < mask.cols))
        {
            if((mask.data != NULL) && (mask.at<uchar>((round)(p2d[0].y), (round)(p2d[0].x)) != 255))
            {
                next_point = false;
            }
        }
        else next_point = false;
        num++;
    }
    return(MAX(0, (num - 2)));
}

int Camera::getImagePoints(cv::Mat &undist_img, uint num,
                   std::vector<cv::Point2f> &img_points)
{
    using namespace std;
    using namespace cv;

    /**************************************** 1. Contour detections *******************************************/
    Mat undist_img_gray;
    cvtColor(undist_img, undist_img_gray, CV_RGB2GRAY); // Convert to grayscale

    Mat temp;
    undist_img_gray(Rect(0, undist_img_gray.rows * (1 - roi) - 10, undist_img_gray.cols, undist_img_gray.rows * roi)).copyTo(temp); // Get roi

    Ptr<CvMemStorage> storage;
    storage = cvCreateMemStorage(0);
    CvSeq * root = cvCreateSeq( 0, sizeof(CvSeq), sizeof(CvSeq*), storage );

    int contours_num =  GetContours(temp, &root, storage, cntrMinSize); // Get contours

    // If number of contours not equal to CONTOURS_NUM, then complete the calibration process
    if(contours_num < CONTOURS_NUM) {
        sec2vector(&root, img_points, Point2f(0, undist_img.rows * (1 - roi) - 10));
        if(contours_num == 0) {
            cout << "Camera " << index << ". No contours were found. Change the calibration image" << endl;
            return(-1);
        }
        cout << "Camera " << index << ". The number of contours is fewer than 4. Change the calibration image" << endl;
        return(-1);
    }
    else if(contours_num > CONTOURS_NUM) {
        sec2vector(&root, img_points, Point2f(0, undist_img.rows * (1 - roi) - 10));
        cout << "Camera " << index << ". The number of contours is bigger than 4. Change the calibration image" << endl;
        return(-1);
    }
    /**************************************** 2. Contour sorting  ********************************************/
    SortContours(&root); // Sort contours from left to right

    /************************************** 3. Get contours points *******************************************/
    Point2f shift = Point2f(0,undist_img.rows * (1 - roi) - 10);
    GetFeaturePoints(&root, img_points, shift); // Sort contour points clockwise (start from the top left point)

    for(int i = 0; i < (int)img_points.size() - 1; i++) {
        line(undist_img, img_points[i], img_points[i+1], cvScalar(255,0,0), 1, CV_AA, 0); // Draw contours
    }

    if (img_points.size() != num) { // Check points count
        cout << "Too few points were found" << endl;
        return(-1);
    }
    return(0);
}
