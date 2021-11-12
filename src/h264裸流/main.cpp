
#include <opencv2/highgui.hpp>
#include <iostream>
extern "C"
{
#include <libswscale/swscale.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}
#pragma comment(lib, "swscale.lib")
#pragma comment(lib, "avcodec.lib")
#pragma comment(lib, "avutil.lib")
#pragma comment(lib, "avformat.lib")
#pragma comment(lib,"opencv_world320.lib")
using namespace std;
using namespace cv;
int main(int argc, char *argv[])
{
	
	//nginx-rtmp ֱ��������rtmp����URL
	const char *outUrl = "udp://127.0.0.1:1234";
	// ffplay "rtmp://127.0.0.1:1935/live/123"
	//ע�����еı������
	avcodec_register_all();

	//ע�����еķ�װ��
	av_register_all();

	//ע����������Э��
	avformat_network_init();


	VideoCapture capture(0);
	Mat frame;


	//���ظ�ʽת��������
	SwsContext *vsc = NULL;

	//��������ݽṹ
	AVFrame *yuv = NULL;

	//������������
	AVCodecContext *vc = NULL;

	//rtmp flv ��װ��
	AVFormatContext *ic = NULL;


	FILE* f264 = fopen("C:/Users/75846/Desktop/out.h264", "wb");


		/// 1 ʹ��opencv��rtsp���

		if (!capture.isOpened())
		{
			throw exception("cam open failed!");
		}
	
		int inWidth = capture.get(CAP_PROP_FRAME_WIDTH);
		int inHeight = capture.get(CAP_PROP_FRAME_HEIGHT);
		int fps = capture.get(CAP_PROP_FPS);//

		///2 ��ʼ����ʽת��������
		vsc = sws_getCachedContext(vsc,
			inWidth, inHeight, AV_PIX_FMT_BGR24,	 //Դ���ߡ����ظ�ʽ
			inWidth, inHeight, AV_PIX_FMT_YUV420P,//Ŀ����ߡ����ظ�ʽ
			SWS_BICUBIC,  // �ߴ�仯ʹ���㷨
			0, 0, 0
			);
		if (!vsc)
		{
			throw exception("sws_getCachedContext failed!");
		}
		///3 ��ʼ����������ݽṹ
		yuv = av_frame_alloc();
		yuv->format = AV_PIX_FMT_YUV420P;
		yuv->width = inWidth;
		yuv->height = inHeight;
		yuv->pts = 0;
		//����yuv�ռ�
		int ret = av_frame_get_buffer(yuv, 32);
		cout << ret << endl;
		if (ret != 0)
		{
			char buf[1024] = { 0 };
			av_strerror(ret, buf, sizeof(buf) - 1);
			throw exception(buf);
		}
	
		///4 ��ʼ������������
		//a �ҵ�������
		AVCodec *codec = avcodec_find_encoder(AV_CODEC_ID_H264);
		if (!codec)
		{
			throw exception("Can`t find h264 encoder!");
		}
		//b ����������������
		vc = avcodec_alloc_context3(codec);
		if (!vc)
		{
			throw exception("avcodec_alloc_context3 failed!");
		}
		//c ���ñ���������
		vc->flags |= AV_CODEC_FLAG_GLOBAL_HEADER; //ȫ�ֲ���
		vc->codec_id = codec->id;
		vc->thread_count = 8;

		vc->bit_rate = 50 * 1024 * 8;//ѹ����ÿ����Ƶ��bitλ��С 50kB
		vc->width = inWidth;
		vc->height = inHeight;
		vc->time_base = { 1,30 };//fpsһֱΪ0
		vc->framerate = { 30,1 };

		//������Ĵ�С������֡һ���ؼ�֡
		vc->gop_size = 50;
		vc->max_b_frames = 0;
		vc->pix_fmt = AV_PIX_FMT_YUV420P;
		//d �򿪱�����������
		ret = avcodec_open2(vc, codec, 0);//codec����0

		if (ret != 0)
		{
			char buf[1024] = { 0 };
			av_strerror(ret, buf, sizeof(buf) - 1);
			throw exception(buf);
		}
		cout << "avcodec_open2 success!" << endl;
		
		///5 �����װ������Ƶ������
		//a ���������װ��������
		ret = avformat_alloc_output_context2(&ic, 0, "flv", outUrl);
		if (ret != 0)
		{
			char buf[1024] = { 0 };
			av_strerror(ret, buf, sizeof(buf) - 1);
			throw exception(buf);
		}
		//b �����Ƶ�� 
		AVStream *vs = avformat_new_stream(ic, NULL);
		if (!vs)
		{
			throw exception("avformat_new_stream failed");
		}
		vs->codecpar->codec_tag = 0;
		//�ӱ��������Ʋ���
		avcodec_parameters_from_context(vs->codecpar, vc);
		av_dump_format(ic, 0, outUrl, 1);


		///��rtmp ���������IO
		ret = avio_open(&ic->pb, outUrl, AVIO_FLAG_WRITE);
		cout << ret;
		
		if (ret != 0)
		{
			char buf[1024] = { 0 };
			av_strerror(ret, buf, sizeof(buf) - 1);
			throw exception(buf);
		}
		
		//д���װͷ
		ret = avformat_write_header(ic, NULL);

		if (ret != 0)
		{
			char buf[1024] = { 0 };
			av_strerror(ret, buf, sizeof(buf) - 1);
			throw exception(buf);
		}
		
		AVPacket pack;
		memset(&pack, 0, sizeof(pack));
		
		int vpts = 0;

		for (;;)
		{
			capture >> frame;
			//imshow("video", frame);
			waitKey(20);

			///rgb to yuv
			//��������ݽṹ
			uint8_t *indata[AV_NUM_DATA_POINTERS] = { 0 };
			//indata[0] bgrbgrbgr
			//plane indata[0] bbbbb indata[1]ggggg indata[2]rrrrr 
			indata[0] = frame.data;
			int insize[AV_NUM_DATA_POINTERS] = { 0 };
			//һ�У������ݵ��ֽ���
			insize[0] = frame.cols * frame.elemSize();
			int h = sws_scale(vsc, indata, insize, 0, frame.rows, //Դ����
				yuv->data, yuv->linesize);
			if (h <= 0)
			{
				continue;
			}
			//cout << h << " " << flush;
	
			///h264����
			yuv->pts = vpts;
			vpts++;
			ret = avcodec_send_frame(vc, yuv);//���߳�




		
			if (ret != 0)
				continue;
			
			ret = avcodec_receive_packet(vc, &pack);
			if (ret != 0 || pack.size > 0)
			{
				if (pack.data) {
					cout << "*" << *pack.data << flush << endl;
				}
				
			}
			
			//����sps,pps��ȫnalu
			if (pack.data != NULL) {

				//���жϱ���֡���ͣ������I֡���ȱ���һ��SPS/PPS���ݡ�������4���ֽ�strat code��NALu��Ԫ����5���ֽڵĺ�5λ������֡���ͣ�I֡��ȡֵΪ5��
				if ((pack.data[4] & 0x1f) == 5) {
					fwrite(vc->extradata, 1, vc->extradata_size, f264);
				}

				//printf("\rSucceed to encode frame %d\n", frameCnt);
				fwrite(pack.data, 1, pack.size, f264);

			}
			
			
			//����
			pack.pts = av_rescale_q(pack.pts, vc->time_base, vs->time_base);
			pack.dts = av_rescale_q(pack.dts, vc->time_base, vs->time_base);
			pack.duration = av_rescale_q(pack.duration, vc->time_base, vs->time_base);
			ret = av_interleaved_write_frame(ic, &pack);
			if (ret == 0)
			{
				cout << "#" << flush;
			}
			
		
		}
		
	
	
	getchar();
	return 0;
}