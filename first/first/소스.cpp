#include <opencv2/opencv.hpp>
#include <vector>

using namespace cv;
using namespace std;


Mat  preprocessing(Mat image)
{
	Mat gray, th_img, morph;
	Mat kernel(5, 15, CV_8UC1, Scalar(1));		// ���� ���� ����ũ
	cvtColor(image, gray, CV_BGR2GRAY);		// ��ϵ� ���� ��ȯ

	blur(gray, gray, Size(5, 5));				// ����
	Sobel(gray, gray, CV_8U, 1, 0, 3);			// �Һ� ���� ����

	threshold(gray, th_img, 120, 255, THRESH_BINARY);	// ����ȭ ����
//	morphologyEx(th_img, morph, MORPH_CLOSE, kernel);	// ���� ���� ����
	morphologyEx(th_img, morph, MORPH_CLOSE, kernel, Point(-1, -1), 2);	// ���� ���� ����
//	imshow("th_img", th_img), imshow("morph", morph);
	return morph;
}

bool vertify_plate(RotatedRect mr)
{
	float size = mr.size.area();
	float aspect = (float)mr.size.height / mr.size.width;	// ��Ⱦ�� ���
	if (aspect < 1)  aspect = 1 / aspect;

	bool  ch1 = size > 2000 && size < 30000;		// ��ȣ�� ���� ����
	bool  ch2 = aspect > 1.3 && aspect < 6.4;		// ��ȣ�� ��Ⱦ�� ����

	return  ch1 && ch2;
}
void stainDetect(Mat temp,Mat img,int i,int j,vector<Point> & coordinate)
{
	if ((int)temp.at<uchar>(i, j) != 0)
		return;
	coordinate.push_back(Point(j, i));


	/*cout << j << " " << i << endl;*/
	temp.at<uchar>(i, j) = 255;//���� �¿�

	if (i != 0)
		if ((int)img.at<uchar>(i - 1, j) != 0)//��
		{
			stainDetect(temp, img,  i-1, j, coordinate);

		}
	if (j != 0)
		if ((int)img.at<uchar>(i, j - 1) != 0)//��
		{
			stainDetect(temp, img, i, j-1, coordinate);

		}
	if (i + 1 == img.rows)//����ó��
		return;
	if ((int)img.at<uchar>(i + 1, j) != 0)//��
	{
		stainDetect(temp, img,  i + 1, j, coordinate);

	}
	if (j + 1 == img.cols)//����ó��
		return;
	if ((int)img.at<uchar>(i, j + 1) != 0)//��
	{
		stainDetect(temp, img, i , j+1, coordinate);

	}
}
void find_candidates(Mat img, vector<RotatedRect>& candidates,Mat ori)
{
	vector< vector< Point> > contours;				// �ܰ���
// �ܰ��� ����
	Mat temp;
	temp = img.clone();//�̹��� ����
	temp.setTo(cv::Scalar(0, 0, 0));//0���� �־���
	int count = 0;

	vector< vector< Point> > mycontours;				// �ܰ���

	for(int i=0;i<img.rows;i++)
	{
		for (int j = 0; j < img.cols; j++)
		{
			if ((int)temp.at<uchar>(i, j) != 0)
				continue;
			if ((int)img.at<uchar>(i, j))//������ �����ΰ� ����� ���
			{
				vector<Point> temp2;

				stainDetect(temp, img, i, j,temp2);
				mycontours.push_back(temp2);

				count++;
				//��
				//��
			}
			
		}
	}
	cout << count << endl;
	vector< vector< Point> > ohmyvector;				// �ܰ���

	for(int i=0;i<mycontours.size();i++)
	{
		vector<Point> temp2;
		temp2.push_back(mycontours[i][0]);
		for(int j=1;j< mycontours[i].size();j++)//ó���� �ϴ� �ְ�
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

	for (int i = 0; i < (int)ohmyvector.size(); i++)	// ���� �ܰ��� ��ȸ
	{
		drawContours(ori, ohmyvector, i, Scalar(0, 255, 255), 2, 8, 0, 0, Point());

		RotatedRect  rot_rect = minAreaRect(ohmyvector[i]);	// �ܰ��� �ּҿ��� ȸ���簢��
		if (vertify_plate(rot_rect))						// ��ȣ�� ����
		{
			candidates.push_back(rot_rect);				// ȸ���簢�� ����


		}
	}
	imwrite("output.jpg", ori);

}
void  refine_candidate(Mat image, RotatedRect& candi)
{
	Mat fill(image.size() + Size(2, 2), CV_8UC1, Scalar(0));   	// ä�� ����
	Scalar  dif1(25, 25, 25), dif2(25, 25, 25);						// ä�� ���� ���� 
	int  flags = 4 + 0xff00;										// ä�� ����
	flags += FLOODFILL_FIXED_RANGE + FLOODFILL_MASK_ONLY;

	// �ĺ����� ���� �÷� ä��
	vector<Point2f> rand_pt(15);						// ���� ��ǥ 15��
	randn(rand_pt, 0, 7);
	Rect img_rect(Point(0, 0), image.size());			// �Է¿��� ���� �簢��
	for (int i = 0; i < rand_pt.size(); i++) {
		Point2f seed = candi.center + rand_pt[i];		// ������ǥ �����̵�
		if (img_rect.contains(seed)) {					// �Է¿��� �����̸�
			floodFill(image, fill, seed, Scalar(), &Rect(), dif1, dif2, flags);
		}
	}

	// ä�� ���� �簢�� ���
	vector<Point> fill_pts;
	for (int i = 0; i < fill.rows; i++) {			// ä�� ��� ���� ��ȸ
		for (int j = 0; j < fill.cols; j++) {
			if (fill.at<uchar>(i, j) == 255) 		// ä�� �����̸� 
				fill_pts.push_back(Point(j, i));		// ��ǥ ����
		}
	}
	candi = minAreaRect(fill_pts);				// ä�� ��ǥ��� �ּҿ��� ���
}

void  rotate_plate(Mat image, Mat& corp_img, RotatedRect candi)
{
	float aspect = (float)candi.size.width / candi.size.height;	// ��Ⱦ�� 
	float angle = candi.angle;									// ȸ������	

	if (aspect < 1) {											// 1���� ������ ���η� �� ����
		swap(candi.size.width, candi.size.height);				// ���� ���� �¹ٲ�
		angle += 90;											// ȸ������ ����
	}

	Mat rotmat = getRotationMatrix2D(candi.center, angle, 1);			// ȸ�� ��� ���
	warpAffine(image, corp_img, rotmat, image.size(), INTER_CUBIC);	// ȸ����ȯ ����
	getRectSubPix(corp_img, candi.size, candi.center, corp_img);
}

vector<Mat> make_candidates(Mat image, vector<RotatedRect>& candidates)
{
	vector<Mat> candidates_img;
	for (int i = 0; i < (int)candidates.size();)
	{
		refine_candidate(image, candidates[i]);		// �ĺ� ���� ����
		if (vertify_plate(candidates[i]))				// ���� ���� �����
		{
			Mat corp_img;
			rotate_plate(image, corp_img, candidates[i]);	// ȸ�� �� �ĺ����� ��������

			cvtColor(corp_img, corp_img, CV_BGR2GRAY); 				// ��ϵ� ��ȯ
			resize(corp_img, corp_img, Size(144, 28), 0, 0, INTER_CUBIC); // ũ�� ����ȭ
			candidates_img.push_back(corp_img);						// ���� ���� ����
			i++;
		}
		else 											// ����� Ż�� 
			candidates.erase(candidates.begin() + i);	// ���� ���ҿ��� ����

	}
	return candidates_img;
}
//#include "preprocess.hpp"
//#include "candiate.hpp"

int main()
{
	int car_no;
	/*cout << "���� ���� ��ȣ (0-20) : ";
	cin >> car_no;

	string fn = format("../image/test_car/%02d.jpg", car_no);*/
	string fn = format("../image/02.jpg");
	Mat image = imread(fn, 1);
	CV_Assert(image.data);

	Mat morph = preprocessing(image);
	vector<RotatedRect> candidates;
	find_candidates(morph, candidates,image);

	vector<Mat> candidate_img = make_candidates(image, candidates);

	// �ĺ����� ǥ�� 
	for (int i = 0; i < candidate_img.size(); i++) {
		imshow("�ĺ�����- " + to_string(i), candidate_img[i]);
		resizeWindow("�ĺ�����- " + to_string(i), 200, 40);		//������ ũ�� ����
	}
	imshow("image - " + to_string(1), image);
	waitKey();
	return 0;
}