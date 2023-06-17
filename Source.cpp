#include <opencv2/opencv.hpp> 
#include <iostream> 

using namespace std;
using namespace cv;

int clickCounter=0;
double coinLength, Actual_coinwidth;

// Global variables to store the two points selected by the user
Point2f point1(-1, -1), point2(-1, -1);

// Mouse Call Back event used to define two functions for selecting points and image magnification
void mouseSP(int event, int x, int y, int flags, void* userdata) {
    static bool magnify = false;
    static const int MAG_FACTOR = 2;
    static const Size MAG_SIZE(100, 100);
    static Mat magnified;

    Mat& image = *(Mat*)userdata;
    int width = image.cols;
    int height = image.rows;

    // Create a magnified window that consists of a crosshair assisting for the point selection
    if (event == EVENT_MOUSEMOVE) {
        if (!magnify && x >= 0 && x < width && y >= 0 && y < height) {
            magnify = true;
            Rect roi(x - MAG_SIZE.width / 2, y - MAG_SIZE.height / 2, MAG_SIZE.width, MAG_SIZE.height);
            Mat tmp;
            getRectSubPix(image, roi.size(), Point2f(x, y), tmp);
            resize(tmp, magnified, Size(MAG_SIZE.width * MAG_FACTOR, MAG_SIZE.height * MAG_FACTOR), INTER_LINEAR);
            namedWindow("Magnified");
            imshow("Magnified", magnified);   
            destroyWindow("Magnified");
        }
        else if (magnify && (x < 0 || x >= width || y < 0 || y >= height)) {
            magnify = false;
            destroyWindow("Magnified");
        }
        else if (magnify) {
            Rect roi(x - MAG_SIZE.width / 2, y - MAG_SIZE.height / 2, MAG_SIZE.width, MAG_SIZE.height);
            Mat tmp;
            getRectSubPix(image, roi.size(), Point2f(x, y), tmp);
            resize(tmp, magnified, Size(MAG_SIZE.width * MAG_FACTOR, MAG_SIZE.height * MAG_FACTOR), INTER_LINEAR);
            line(magnified, Point(100, 80), Point(100, 120), Scalar(255, 255, 0), 1);
            line(magnified, Point(80, 100), Point(120, 100), Scalar(255, 255, 0), 1);
            imshow("Magnified", magnified);
        }
    }

    // Select two points within the image and store it as point1 and point2
    if (event == EVENT_LBUTTONDOWN && clickCounter == 0) {
        if (!point1.x && !point1.y) {
            point1 = Point(x, y);
            cout << "First point recorded: (" << x << ", " << y << ")" << endl;
            clickCounter++;
        }
    }
    else if (event == EVENT_LBUTTONDOWN && clickCounter == 1) {
        if (!point2.x && !point2.y) {
            point2 = Point(x, y);
            cout << "Second point recorded: (" << x << ", " << y << ")" << endl;
            clickCounter++;
        }
    }
}

// Function to compute the midpoint between two points
Point2f midpoint(const Point2f& p1, const Point2f& p2) {
    return Point2f((p1.x + p2.x) / 2.0, (p1.y + p2.y) / 2.0);
}

// Function to determine the object shape type by taking total number or corners into account
void getShape(Mat imgDil, Mat img) {
    vector<vector<Point>> contours;
    findContours(imgDil, contours, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);

    for (size_t i = 0; i < contours.size(); i++) {
        int area = contourArea(contours[i]);
        //cout << area << endl;

        vector<vector<Point>> conPoly(contours.size());
        vector<Rect> boundRect(contours.size());
        string objectType;

        if (area > 1000) {
            float peri = arcLength(contours[i], true);
            approxPolyDP(contours[i], conPoly[i], 0.02 * peri, true);
            //cout << conPoly[i].size() << endl;
            boundRect[i] = boundingRect(conPoly[i]);
            int objCor = (int)conPoly[i].size();

            if (objCor == 3) { objectType = "Triangle"; }
            if (objCor == 4) {
                float aspRatio = (float)boundRect[i].width / (float)boundRect[i].height;
                //cout << aspRatio << endl;
                if (aspRatio > 0.95 && aspRatio < 1.05) { objectType = "Square"; }
                else { objectType = "Rectangle"; }
            }
            if (objCor == 5) { objectType = "Pentagon"; }
            if (objCor > 5) { objectType = "Circle"; }

            //drawContours(img, contours, -1, Scalar(255, 0, 0), 2);
            //rectangle(img, boundRect[i].tl(), boundRect[i].br(), Scalar(0, 255, 0), 3);
            putText(img, objectType, { boundRect[i].x,boundRect[i].y - 5 }, FONT_HERSHEY_COMPLEX, 1, Scalar(0, 0, 100), 1);
        }
    }
}

int main(int argc, char** argv) {
    // Load the image 
    Mat image = imread("Test_1.jpeg", IMREAD_COLOR);
    if (image.empty()) {
        cout << "Could not open or find the image" << endl;
        return -1;
    }

    // User Input to store the width of an object or a coin diameter
    cout << "Enter the Coin_Diameter/Width_of_Any_Object (in Centimeters) to calibrate the Pixel_Ratio \n(For Ex. 1 Pound: 2.34 cms & 2 Pounds: 2.84 cms): ";
    cin >> Actual_coinwidth;

    // Image is scaled using the resize function but the scaling factor is set to 1, so the image won't be affected
    Mat scaled;
    resize(image, scaled, Size(), 1, 1);

    // A window named Select Points is created, where the mousecallback is used to select points along with the magnification window
    namedWindow("Select Points", WINDOW_NORMAL);
    setMouseCallback("Select Points", mouseSP, &image);
    
    // A loop is created to determine if 2 points are clicked
    while (clickCounter < 3) {
        imshow("Select Points", image);
        if (point1.x && point2.x) {
            line(image, point1, point2, Scalar(255, 255, 255), 2);
            coinLength = norm(point1 - point2);
            
            ostringstream coinDia;
            coinDia << fixed << setprecision(1) << Actual_coinwidth;
            putText(image, coinDia.str() + " cms", Point(point2.x+10, point2.y), FONT_HERSHEY_SIMPLEX, 0.65, Scalar(0, 255, 0), 2);

            point1 = Point(0, 0);
            point2 = Point(0, 0);
        }
        if (waitKey(20) == 13) { break; }
    }
    destroyWindow("Magnified");
    destroyWindow("Select Points");
    
    cout << "Coin Diameter: " << coinLength << " in Pixels" << endl;

    double pixelRatio = coinLength / Actual_coinwidth;

    //-----------------------------------------------------------
    // Apply image preprocessing techniques 
    // Convert the image to grayscale 
    Mat gray;
    cvtColor(scaled, gray, COLOR_BGR2GRAY);
    
    // Example: Gaussian blurring to reduce noise 
    Mat blur_img;
    GaussianBlur(gray, blur_img, Size(5, 5), 0, 0);
    
    // Apply edge detection using Canny algorithm 
    Mat edges_img;
    Canny(blur_img, edges_img, 60, 250, 3);
        
    // Create a structuring element for dilation and erosion 
    Mat element = getStructuringElement(MORPH_RECT, Size(3, 3));

    // Dilate the image 
    Mat dilated_img;
    dilate(edges_img, dilated_img, element);
    
    // Erode the image 
    Mat eroded_img;
    erode(dilated_img, eroded_img, element);
    //-----------------------------------------------------------

    // Apply the Hough Circle Transform 
    vector<Vec3f> circles;
    HoughCircles(edges_img, circles, HOUGH_GRADIENT, 1, 100, 100, 35, 1, 100);
   
    // Draw the circles on the original image 
    for (size_t i = 0; i < circles.size(); i++)
    {
        Point center(cvRound(circles[i][0]), cvRound(circles[i][1]));
        double radius = cvRound(circles[i][2]);
        circle(image, center, radius, Scalar(255, 0, 0), 2);
        double actual_radius = radius / pixelRatio;
        double actual_diameter = actual_radius * 2;

        ostringstream ss;
        ss << fixed << setprecision(2) << "Radius: " << actual_radius << " cms" << " & " << "Diameter: " << actual_diameter << " cms";
        putText(image, ss.str(), Point(center.x-radius-100, center.y - radius - 30), FONT_HERSHEY_SIMPLEX, 0.5, Scalar(0, 255, 255), 2);
    }

    // Determine the contours along the edges of an object using the findContours() function
    vector<vector<Point>> contours;
    findContours(eroded_img, contours, RETR_LIST, CHAIN_APPROX_SIMPLE);

    // Draw rotated rectangles over the contours on the original image 
    Mat result = image.clone();
    Point2f p1, p2;

    getShape(eroded_img, result);

    for (size_t i = 0; i < contours.size(); ++i)
    {
        // Find the minimum bounding rectangle of the contour 
        RotatedRect rect = minAreaRect(contours[i]);

        // Draw the rectangle on the original image 
        Point2f vertices[4];
        rect.points(vertices);
        for (int j = 0; j < 4; ++j)
        {
            double w = rect.size.width, h = rect.size.height;
            if (w > result.cols / 10.0 && h > result.rows / 10.0)
            {
                line(result, vertices[j], vertices[(j + 1) % 4], Scalar(0, 255, 0), 2);
                p1 = vertices[j];
                p2 = vertices[(j + 1) % 4];

                vector<vector<Point>> orderedBox(1);
                for (int i = 0; i < 4; i++) {
                    orderedBox[0].push_back(vertices[i]);
                }
                drawContours(result, orderedBox, -1, Scalar(0, 255, 0), 2);

                // loop over the original points and draw them
                for (const auto& pt : orderedBox[0]) {
                    circle(result, pt, 3, Scalar(0, 0, 0), -1);
                }

                // unpack the ordered bounding box, then compute the midpoint
                // between the top-left and top-right coordinates, followed by
                // the midpoint between bottom-left and bottom-right coordinates
                Point2f tl = orderedBox[0][0];
                Point2f tr = orderedBox[0][1];
                Point2f br = orderedBox[0][2];
                Point2f bl = orderedBox[0][3];
                Point2f tltr = midpoint(tl, tr);
                Point2f blbr = midpoint(bl, br);

                // compute the midpoint between the top-left and bottom-left points,
                // followed by the midpoint between the top-right and bottom-right
                Point2f tlbl = midpoint(tl, bl);
                Point2f trbr = midpoint(tr, br);

                // draw lines between the vertices
                line(result, tl, tr, Scalar(255, 0, 255), 2);
                line(result, tr, br, Scalar(255, 0, 255), 2);

                // draw the midpoints on the image
                circle(result, tltr, 3, Scalar(0, 0, 0), -1);
                circle(result, blbr, 3, Scalar(0, 0, 0), -1);
                circle(result, tlbl, 3, Scalar(0, 0, 0), -1);
                circle(result, trbr, 3, Scalar(0, 0, 0), -1);

                // compute the Euclidean distance between the midpoints
                float dA = norm((tltr, tltr) - (blbr, blbr));
                float dB = norm((tlbl, tlbl) - (trbr, trbr));
                
                // compute the size of the object
                float dimA = dA / pixelRatio;
                float dimB = dB / pixelRatio;
                ostringstream dimA_str;
                dimA_str << fixed << setprecision(1) << dimA;
                ostringstream dimB_str;
                dimB_str << fixed << setprecision(1) << dimB;

                if(j % 2){ 
                    putText(result, dimA_str.str() + " cms", Point(trbr.x - 50, trbr.y - 10), FONT_HERSHEY_SIMPLEX, 0.65, Scalar(255, 255, 0), 2);
                    putText(result, dimB_str.str() + " cms", Point(tltr.x - 50, tltr.y - 10), FONT_HERSHEY_SIMPLEX, 0.65, Scalar(255, 255, 0), 2);
                }
            }
        }
    }

    // Show the result 
    imshow("Result", result);
    waitKey(0);

    destroyAllWindows();
    return 0;

}