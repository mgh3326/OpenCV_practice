#include <opencv2/opencv.hpp>
#include <vector>

using namespace cv;
using namespace std;


Mat  preprocessing(Mat image)
{
	Mat gray, th_img, morph;
	Mat kernel(5, 15, CV_8UC1, Scalar(1));		// 닫힘 연산 마스크
	cvtColor(image, gray, CV_BGR2GRAY);		// 명암도 영상 변환

	blur(gray, gray, Size(5, 5));				// 블러링
	Sobel(gray, gray, CV_8U, 1, 0, 3);			// 소벨 에지 검출

	threshold(gray, th_img, 120, 255, THRESH_BINARY);	// 이진화 수행
//	morphologyEx(th_img, morph, MORPH_CLOSE, kernel);	// 열림 연산 수행
	morphologyEx(th_img, morph, MORPH_CLOSE, kernel, Point(-1, -1), 2);	// 열림 연산 수행
//	imshow("th_img", th_img), imshow("morph", morph);
	return morph;
}

bool vertify_plate(RotatedRect mr)
{
	float size = mr.size.area();
	float aspect = (float)mr.size.height / mr.size.width;	// 종횡비 계산
	if (aspect < 1)  aspect = 1 / aspect;

	bool  ch1 = size > 2000 && size < 30000;		// 번호판 넓이 조건
	bool  ch2 = aspect > 1.3 && aspect < 6.4;		// 번호판 종횡비 조건

	return  ch1 && ch2;
}
void stainDetect(Mat temp,Mat img,int i,int j,vector<Point> & coordinate)
{
	if ((int)temp.at<uchar>(i, j) != 0)
		return;
	coordinate.push_back(Point(j, i));


	/*cout << j << " " << i << endl;*/
	temp.at<uchar>(i, j) = 255;//상하 좌우

	if (i != 0)
		if ((int)img.at<uchar>(i - 1, j) != 0)//상
		{
			stainDetect(temp, img,  i-1, j, coordinate);

		}
	if (j != 0)
		if ((int)img.at<uchar>(i, j - 1) != 0)//좌
		{
			stainDetect(temp, img, i, j-1, coordinate);

		}
	if (i + 1 == img.rows)//예외처리
		return;
	if ((int)img.at<uchar>(i + 1, j) != 0)//하
	{
		stainDetect(temp, img,  i + 1, j, coordinate);

	}
	if (j + 1 == img.cols)//예외처리
		return;
	if ((int)img.at<uchar>(i, j + 1) != 0)//상
	{
		stainDetect(temp, img, i , j+1, coordinate);

	}
}
void find_candidates(Mat img, vector<RotatedRect>& candidates,Mat ori)
{
	vector< vector< Point> > contours;				// 외곽선
// 외곽선 검출
	Mat temp;
	temp = img.clone();//이미지 복사
	temp.setTo(cv::Scalar(0, 0, 0));//0값을 넣어줌
	int count = 0;

	vector< vector< Point> > mycontours;				// 외곽선

	for(int i=0;i<img.rows;i++)
	{
		for (int j = 0; j < img.cols; j++)
		{
			if ((int)temp.at<uchar>(i, j) != 0)
				continue;
			if ((int)img.at<uchar>(i, j))//검은색 외적인게 검출될 경우
			{
				vector<Point> temp2;

				stainDetect(temp, img, i, j,temp2);
				mycontours.push_back(temp2);

				count++;
				//좌
				//우
			}
			
		}
	}
	cout << count << endl;
	vector< vector< Point> > ohmyvector;				// 외곽선

	for(int i=0;i<mycontours.size();i++)
	{
		vector<Point> temp2;
		temp2.push_back(mycontours[i][0]);
		for(int j=1;j< mycontours[i].size();j++)//처음껀 일단 넣고
		{
			if(mycontours[i][j-1].x- mycontours[i][j].x!=0)
			{
				if(temp2.size()>1)
				{
					if (temp2[temp2.size() - 2].y - mycontours[i][j].y == 0&& temp2[temp2.size() - 1].y - mycontours[i][j].y == 0)
					{
						temp2.pop_back();
						temp2.push_back(mycontours[i][j]);

					}
					else
					{
						temp2.push_back(mycontours[i][j]);

					}

				}
				else
				{
					temp2.push_back(mycontours[i][j]);

				}
				

			}
			

		}
		ohmyvector.push_back(temp2);
	}
	findContours(img.clone(), contours, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);

	for (int i = 0; i < (int)ohmyvector.size(); i++)	// 검출 외곽선 조회
	{
		drawContours(ori, ohmyvector, i, Scalar(0, 255, 255), 2, 8, 0, 0, Point());

		RotatedRect  rot_rect = minAreaRect(ohmyvector[i]);	// 외곽선 최소영역 회전사각형
		if (vertify_plate(rot_rect))						// 번호판 검증
		{
			candidates.push_back(rot_rect);				// 회전사각형 저장


		}
	}
	imwrite("output.jpg", ori);

}
void  refine_candidate(Mat image, RotatedRect& candi)
{
	Mat fill(image.size() + Size(2, 2), CV_8UC1, Scalar(0));   	// 채움 영역
	Scalar  dif1(25, 25, 25), dif2(25, 25, 25);						// 채움 색상 범위 
	int  flags = 4 + 0xff00;										// 채움 방향
	flags += FLOODFILL_FIXED_RANGE + FLOODFILL_MASK_ONLY;

	// 후보영역 유사 컬러 채움
	vector<Point2f> rand_pt(15);						// 랜덤 좌표 15개
	randn(rand_pt, 0, 7);
	Rect img_rect(Point(0, 0), image.size());			// 입력영상 범위 사각형
	for (int i = 0; i < rand_pt.size(); i++) {
		Point2f seed = candi.center + rand_pt[i];		// 랜덤좌표 평행이동
		if (img_rect.contains(seed)) {					// 입력영상 범위이면
			floodFill(image, fill, seed, Scalar(), &Rect(), dif1, dif2, flags);
		}
	}

	// 채움 영역 사각형 계산
	vector<Point> fill_pts;
	for (int i = 0; i < fill.rows; i++) {			// 채움 행렬 원소 조회
		for (int j = 0; j < fill.cols; j++) {
			if (fill.at<uchar>(i, j) == 255) 		// 채움 영역이면 
				fill_pts.push_back(Point(j, i));		// 좌표 저장
		}
	}
	candi = minAreaRect(fill_pts);				// 채움 좌표들로 최소영역 계산
}

void  rotate_plate(Mat image, Mat& corp_img, RotatedRect candi)
{
	float aspect = (float)candi.size.width / candi.size.height;	// 종횡비 
	float angle = candi.angle;									// 회전각도	

	if (aspect < 1) {											// 1보다 작으면 세로로 긴 영역
		swap(candi.size.width, candi.size.height);				// 가로 세로 맞바꿈
		angle += 90;											// 회전각도 조정
	}

	Mat rotmat = getRotationMatrix2D(candi.center, angle, 1);			// 회전 행렬 계산
	warpAffine(image, corp_img, rotmat, image.size(), INTER_CUBIC);	// 회전변환 수행
	getRectSubPix(corp_img, candi.size, candi.center, corp_img);
}

vector<Mat> make_candidates(Mat image, vector<RotatedRect>& candidates)
{
	vector<Mat> candidates_img;
	for (int i = 0; i < (int)candidates.size();)
	{
		refine_candidate(image, candidates[i]);		// 후보 영역 개선
		if (vertify_plate(candidates[i]))				// 개선 영역 재검증
		{
			Mat corp_img;
			rotate_plate(image, corp_img, candidates[i]);	// 회전 및 후보영상 가져오기

			cvtColor(corp_img, corp_img, CV_BGR2GRAY); 				// 명암도 변환
			resize(corp_img, corp_img, Size(144, 28), 0, 0, INTER_CUBIC); // 크기 정규화
			candidates_img.push_back(corp_img);						// 보정 영상 저장
			i++;
		}
		else 											// 재검증 탈락 
			candidates.erase(candidates.begin() + i);	// 벡터 원소에서 제거

	}
	return candidates_img;
}
//#include "preprocess.hpp"
//#include "candiate.hpp"

int main()
{
	int car_no;
	/*cout << "차량 영상 번호 (0-20) : ";
	cin >> car_no;

	string fn = format("../image/test_car/%02d.jpg", car_no);*/
	string fn = format("../image/02.jpg");
	Mat image = imread(fn, 1);
	CV_Assert(image.data);

	Mat morph = preprocessing(image);
	vector<RotatedRect> candidates;
	find_candidates(morph, candidates,image);

	vector<Mat> candidate_img = make_candidates(image, candidates);

	// 후보영상 표시 
	for (int i = 0; i < candidate_img.size(); i++) {
		imshow("후보영상- " + to_string(i), candidate_img[i]);
		resizeWindow("후보영상- " + to_string(i), 200, 40);		//윈도우 크기 조정
	}
	imshow("image - " + to_string(1), image);
	waitKey();
	return 0;
}