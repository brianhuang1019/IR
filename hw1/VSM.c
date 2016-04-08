#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <wctype.h>
#include <math.h>
#include <ctype.h>
#include <locale.h>

#define maxdoc 50000
#define maxvocab 30000
#define DOC_NUM 46972
#define VOC_NUM 29908 	//2.23 1 0.0253 0.8 //2.75 1 0.0253 0.8 
#define k 2.75 		//2.23
#define b 1
#define alpha 0.0253

typedef struct {
	int voc1;
	int voc2;
	double rele_count; 	// use to calculate the relevance in relevance feedback 
	double tf;			// term frequency in this doc
	double tf_norm;
	double df; 			// document frequence of the term
	double weight;
} termS;

typedef struct {
	wchar_t doc_ID[100];
	int max_t;
	int curr_t;
	termS* term;
	double similarity;
	double cosinSp1;
	double cosinSp2;
	double cosine;
	int check;
} docS;

wchar_t vocab[maxvocab][100]; 	// total vocab number with length maximum 100
docS doc[maxdoc]; 				// total document number
docS *query; 					// query document

int cmp (const void *a1, const void *a2) {
	termS *t1 = (termS*)a1;
	termS *t2 = (termS*)a2;
	if (t1->voc1 < t2->voc1) 
		return -1;
	else if (t1->voc1 > t2->voc1)
		return 1;
	else {
		if (t1->voc2 < t2->voc2)
			return -1;
		else
			return 1;
	}
}

double count_IDF(double doc_frequency) {
	return log((double)(DOC_NUM + 0.5) / (doc_frequency + 0.5));
}

double count_TF(double term_frequency, int curr_t, double avgdoclen) {
	return (k + 1) * term_frequency / (term_frequency + k * (1 - b + b * curr_t / avgdoclen));
}

// cut off useless words in query
void cut_query(wchar_t* query_buffer) {
	wchar_t *useless;
	useless = wcsstr(query_buffer, L"查詢");
	if (useless != NULL)
		wcscpy(query_buffer, useless+2);
	useless = wcsstr(query_buffer, L"有關");
	if (useless != NULL) {
		*useless = L'\0';
		wcscat(query_buffer, useless+2);
	}
	useless = wcschr(query_buffer, L'，');
	if (useless != NULL) {
		*useless = L'\0';
		wcscat(query_buffer, useless+1);
	}
	useless = wcschr(query_buffer, L'。');
	if (useless != NULL) {
		*useless = L'\0';
	}
}

int main(int argc, char** argv) {
	setlocale(LC_ALL, "zh_TW.UTF-8");
	int i = 0, j = 0;
	int hum_f_flag = 0;

	int relevance_fb = 0;
	char query_file[50];
	char output_file[50] = "ranked-list";
	char model_dir[50];
	char NTCIR_dir[50];
	char tmp[100];
	// double hum_f[20][3];
	// hum_f[0][0] = -0.000747;
	// hum_f[1][0] = 0.000171;
	// hum_f[2][0] = 0.00121;
	// hum_f[3][0] = -0.0004;
	// hum_f[4][0] = 0.000542;
	// hum_f[5][0] = -0.000201;
	// hum_f[6][0] = 0.000378;
	// hum_f[7][0] = 0;
	// hum_f[8][0] = 0.000966;
	// hum_f[9][0] = 0.000108;
	// hum_f[10][0] = 0;
	// hum_f[11][0] = -0.000272;
	// hum_f[12][0] = -0.000856;
	// hum_f[13][0] = 0.00115;
	// hum_f[14][0] = -0.000354;
	// hum_f[15][0] = 0;
	// hum_f[16][0] = -0.00048;
	// hum_f[17][0] = -0.000436;
	// hum_f[18][0] = -0.000297;
	// hum_f[19][0] = 0.000186;

	// hum_f[0][1] =-0.000312;
	// hum_f[1][1] = 0;
	// hum_f[2][1] = 0.00148;
	// hum_f[3][1] = 0;
	// hum_f[4][1] = 0.00063;
	// hum_f[5][1] = -0.000551;
	// hum_f[6][1] = 0;
	// hum_f[7][1] = 0;
	// hum_f[8][1] = -0.000129;
	// hum_f[9][1] = 0.000497;
	// hum_f[10][1] = -0.000101;
	// hum_f[11][1] = 0.000487;
	// hum_f[12][1] = 0.000236;
	// hum_f[13][1] = 0.00077;
	// hum_f[14][1] = -0.000471;
	// hum_f[15][1] = 0;
	// hum_f[16][1] = -0.000691;
	// hum_f[17][1] = -0.000314;
	// hum_f[18][1] = 0.000261;
	// hum_f[19][1] = -0.000447;

	FILE *query_fp,			// query file
		 *output_fp,		// output file 
		 *model_voc_fp, 	// vocab.all
		 *model_file_fp, 	// file-list
		 *model_index_fp;	// inverted-file

	// process input parameters
	while (i < argc) {
		if(strcmp(argv[i], "-r") == 0) {
			relevance_fb = 1; // open relevance feedback
		}
		else if(strcmp(argv[i], "-i") == 0) { //specify query file
			strcpy(query_file,argv[++i]);
		}
		else if(strcmp(argv[i], "-o") == 0) { //specify output file
			strcpy(output_file,argv[++i]);
		}
		else if(strcmp(argv[i], "-m") == 0) { //specify modle directory
			strcpy(model_dir,argv[++i]);
		}
		else if(strcmp(argv[i], "-d") == 0) { //specify NTCIR directory
			strcpy(NTCIR_dir,argv[++i]);
		}
		else if(strcmp(argv[i], "-h") == 0) {
			hum_f_flag = 1;
		}
		else {
			//something else
		}
		i++;
	}

	// open file pointers
	strcpy(tmp, "./");
	strcat(tmp, query_file);
	query_fp = fopen(tmp, "r");
	if (query_fp == NULL)
		printf("opening query-file error\n");

	strcpy(tmp, "./");
	strcat(tmp, output_file);
	output_fp = fopen(tmp, "w");
	if (output_fp == NULL)
		printf("creating ranked-list error\n");

	strcpy(tmp, model_dir);
	if (model_dir[strlen(model_dir)-1] != '/')
		strcat(tmp, "/");
	strcat(tmp, "vocab.all"); // only use when we check the query file
	model_voc_fp = fopen(tmp, "r");
	if (model_voc_fp == NULL)
		printf("opening vocab.all error\n");

	strcpy(tmp, model_dir);
	if (model_dir[strlen(model_dir)-1] != '/')
		strcat(tmp, "/");
	strcat(tmp, "file-list");
	model_file_fp = fopen(tmp, "r");
	if (model_file_fp == NULL)
		printf("opening file-list error\n");

	strcpy(tmp, model_dir);
	if (model_dir[strlen(model_dir)-1] != '/')
		strcat(tmp, "/");
	strcat(tmp, "inverted-file");
	model_index_fp = fopen(tmp, "r");
	if (model_index_fp == NULL) 
		printf("opening inverted-file error\n");

	// inital docS and query space
	int default_query_num = 30;
	query = malloc(sizeof(docS) * default_query_num);

	for (i = 0; i < DOC_NUM; i++) {
		doc[i].max_t = 5;
		doc[i].curr_t = 0;
		doc[i].term = malloc(sizeof(termS) * 5);
	}

	// parse vocab.all
	i = 0;
	while (fwscanf(model_voc_fp, L"%ls", &vocab[i]) != EOF) {
		// wprintf(L"%ls",vocab[i]);
		i++;
	}
	fclose(model_voc_fp);
	printf(" [O] Parsing vocab.all finish\n");

	// parse file-list
	for (i = 0; i < DOC_NUM; i++) {
		// fscanf(model_file_fp, "%s", &doc[i].doc_ID);
		// printf("%s\n", doc[i].doc_ID);
		fwscanf(model_file_fp, L"%ls", &doc[i].doc_ID);
	}
	fclose(model_file_fp);
	printf(" [O] Parsing file-list finish\n");

	// parse inverted-file
	int voc1, voc2, docnum;
	int doc_id, frequency;
	int cur;
	while (fscanf(model_index_fp, "%d %d %d", &voc1, &voc2, &docnum) != EOF) {
		for (i = 0; i < docnum; i++) {
			fscanf(model_index_fp, "%d %d", &doc_id, &frequency);
			doc[doc_id].curr_t += 1;
			if (doc[doc_id].curr_t > doc[doc_id].max_t) {	// space not enough
				doc[doc_id].max_t *= 2; 
				doc[doc_id].term = realloc(doc[doc_id].term, sizeof(termS) * doc[doc_id].max_t);
			}
			cur = doc[doc_id].curr_t - 1;
			doc[doc_id].term[cur].voc1 = voc1;
			doc[doc_id].term[cur].voc2 = voc2;
			doc[doc_id].term[cur].tf = frequency;	// term frequence in this doc
			doc[doc_id].term[cur].df = docnum; 		// doc frequence
		}
	}
	fclose(model_index_fp);
	printf(" [O] Parsing inverted-file finish\n");

	// calculate average document length
	double avgdoclen = 0;
	for (i = 0; i < DOC_NUM; i++) {
		avgdoclen += doc[i].curr_t;
	}
	avgdoclen = avgdoclen / DOC_NUM;
	printf(" [O] Calculate avgdoclen finish, it equals to: %lf\n", avgdoclen);

	// calculate term weight (using tf-IDF, tf with Okapi/BM25)
	for (i = 0; i < DOC_NUM; i++) {
		for (j = 0; j < doc[i].curr_t; j++) {
			doc[i].term[j].tf_norm = count_TF(doc[i].term[j].tf, doc[i].curr_t, avgdoclen);
			doc[i].term[j].weight = count_IDF(doc[i].term[j].df) * doc[i].term[j].tf_norm;
		}
	}
	printf(" [O] Calculate term weight using tf-IDF finish\n");

	// pasre XML query
	wchar_t c, cn;
	int query_index, query_num = 0;
	wchar_t narr_buffer[1000], query_buffer[200], tmp_buffer[1000];
	while ((c = fgetwc(query_fp)) != EOF) {
		if (c == L'<') {
			c = fgetwc(query_fp);
			if (c == L'n') {
				c = fgetwc(query_fp);
				if (c == L'u') {	// <number>
					query_num += 1;
					query_index = query_num - 1;
					if (query_num > default_query_num) {
						default_query_num *= 2;
						query = realloc(query, sizeof(docS) * default_query_num);
					}
					query[query_index].max_t = 30000;
					query[query_index].curr_t = 0;
					query[query_index].term = malloc(sizeof(termS) * 30000);

					// parse query name
					wchar_t buffer[100];
					fwscanf(query_fp, L"%ls", &buffer);
					wchar_t *name_start = wcschr(buffer, L'>');
					name_start++;
					wchar_t *name_end = wcschr(buffer, L'<');
					*name_end = L'\0';
					wcscpy(query[query_index].doc_ID, name_start);
					//wprintf(L"%ls\n", query[query_index].doc_ID);
				}	
			}
			else if (c == L'q') {	// <question>
				c = fgetwc(query_fp);
				if (c == L'u') {
					while ((c = fgetwc(query_fp)) != L'\n');
					fwscanf(query_fp, L"%ls", &query_buffer);
					// cut useless words in query
					cut_query(query_buffer);
					// wprintf(L"%ls\n", query_buffer);

					int v1, v2;
					for (i = 0; i < wcslen(query_buffer) - 1; i++) {
						for (j = 0; j < VOC_NUM; j++) {
							if (vocab[j][0] == query_buffer[i])
								break;	
						}
						v1 = j;
						//wprintf(L"%lc",vocab[v1][0]);
						for (j = 0; j < VOC_NUM; j++) {
							if(vocab[j][0] == query_buffer[i+1])
								break;	
						}
						v2 = j;
						//wprintf(L"%lc",vocab[v2][0]);

						// avoid puting same term into set
						for (j = 0; j < query[query_index].curr_t; j++) {
							if (query[query_index].term[j].voc1 == v1 && query[query_index].term[j].voc2 == v2) {
								if (query[query_index].term[query[query_index].curr_t].tf <= 0)
									query[query_index].term[query[query_index].curr_t].tf = 1;
								else
									query[query_index].term[query[query_index].curr_t].tf += 1; //get more weight form question
								break;
							}
						}
						if (j == query[query_index].curr_t) { // if not, then put this term into set
							query[query_index].term[query[query_index].curr_t].voc1 = v1;
							query[query_index].term[query[query_index].curr_t].voc2 = v2;
							query[query_index].term[query[query_index].curr_t].tf = 1; //get more weight form question
							query[query_index].curr_t += 1;
						}
					}
				}
			}
			else if(c == L'c') {
				c = fgetwc(query_fp);
				if(c == L'o') {	// <concept>
					while((c = fgetwc(query_fp)) != L'\n');
					while((c = fgetwc(query_fp)) != L'<') {
						cn = fgetwc(query_fp);
						ungetwc(cn, query_fp);
						int v1,v2;
						if(c==L'、' || c==L'。' || c==L'<' || cn==L'<')
							continue;
						for(i=0;i<VOC_NUM;i++) {
							if(vocab[i][0]==c)
								break;	
						}
						v1 = i;
						//wprintf(L"%lc",vocab[v1][0]);
						for(i=0;i<VOC_NUM;i++) {
							if(vocab[i][0]==cn)
								break;	
						}
						if(cn==L'、' || cn==L'。')
							v2 = -1;
						else
							v2 = i;
						//wprintf(L"%lc ",vocab[v2][0]);
						for(i=0;i<query[query_index].curr_t;i++) { //check if we put same term int to set 
							if(query[query_index].term[i].voc1==v1 && query[query_index].term[i].voc2==v2) {
								if(query[query_index].term[query[query_index].curr_t].tf<=0)
									query[query_index].term[query[query_index].curr_t].tf = 1;
								else
									query[query_index].term[query[query_index].curr_t].tf += 1;
								break;
							}
						}
						if(i==query[query_index].curr_t) { // if not, then put this term into set
							query[query_index].term[query[query_index].curr_t].voc1 = v1;
							query[query_index].term[query[query_index].curr_t].voc2 = v2;
							query[query_index].term[query[query_index].curr_t].tf = 1;
							query[query_index].term[query[query_index].curr_t].rele_count = 0;
							query[query_index].curr_t += 1;
							//printf("%d %d\n",v1,v2);
						}
						//wprintf(L"%lc %lc %d %d\n",c,cn,v1,v2);
					}
				}
			}
			//printf("\n");
		}
	}
	fclose(query_fp);
	//sort every term in each doc by thier ID num
	for(i=0;i<query_num;i++) {
		qsort(query[i].term, query[i].curr_t, sizeof(termS), cmp);
	}
	printf("querynum: %d\n", query_num);
	/*for(i=0;i<DOC_NUM;i++) {
		qsort(doc[i].term, doc[i].curr_t, sizeof(termS), cmp);
	}*/
	//to calculate similarity with each query file//
	int q;
	for (q = 0; q < query_num; q++) {
		int p1, p2; //two pointer in two list to find same vector
		for (i = 0; i < DOC_NUM; i++) {
			p1=0;
			p2=0;
			doc[i].similarity = 0;
			doc[i].cosinSp1 = 0;
			doc[i].cosinSp2 = 0;
			doc[i].check = 0; //in order to be faster in ranking
			while(p1 < doc[i].curr_t && p2 < query[q].curr_t) {// if overload break, else find same term
				if(doc[i].term[p1].voc1 > query[q].term[p2].voc1)
					p2++;
				else if(doc[i].term[p1].voc1 < query[q].term[p2].voc1)
					p1++;
				else {
					if(doc[i].term[p1].voc2 > query[q].term[p2].voc2)
						p2++;
					else if(doc[i].term[p1].voc2 < query[q].term[p2].voc2)
						p1++;
					else if(doc[i].term[p1].voc2==-1 && query[q].term[p2].voc2==-1) { //only consider bi-nary
						p1++;
						p2++;
					}
					else {
						doc[i].similarity += (doc[i].term[p1].weight*1*query[q].term[p2].tf);
						//doc[i].cosinSp1 += doc[i].term[p1].weight*doc[i].term[p1].weight;
						//doc[i].cosinSp2 += query[q].term[p2].tf*query[q].term[p2].tf;
						p1++;
						p2++;
					}
				}
			}
			p1 = 0;
			p2 = 0;
			while(p1 < doc[i].curr_t) {
				if(doc[i].term[p1].voc2!=-1) {
					doc[i].cosinSp1 += doc[i].term[p1].weight*doc[i].term[p1].weight;
				}
				p1++;
			}
			while(p2 < query[q].curr_t) {
				if(query[q].term[p2].voc2!=-1) {
					doc[i].cosinSp2 += query[q].term[p2].tf*query[q].term[p2].tf;
				}
				p2++;
			}
			if(doc[i].cosinSp1!=0 && doc[i].cosinSp2!=0)
				doc[i].cosine = doc[i].similarity/(sqrt(doc[i].cosinSp1)*sqrt(doc[i].cosinSp2)); // cosine similarity
			else
				doc[i].cosine = 0;
		}
		// generate top 100 anwser for each query//
		int top[100];
		double Maxsimi;
		for(i=0;i<100;i++) {
			Maxsimi = 0;
			for(j=0;j<DOC_NUM;j++) {
				if(Maxsimi<doc[j].cosine && doc[j].check==0) {
					Maxsimi = doc[j].cosine;
					top[i] = j;
				}
			}
			doc[top[i]].check = 1; // find top i and set check bit to 1, to exclude it.
			//if(i>0 && wcscmp(doc[top[i]].doc_ID,doc[top[i-1]].doc_ID)==0)
			//	i--;
		}
		//if relevance feeback is specified
		double R = 2; // assume that top 10 is truly relative
		// if(relevance_fb) {
		// 	int p1,p2;
		// 	for(i=0;i<R;i++) {
		// 		p1 = 0;
		// 		p2 = 0;
		// 		while(p1 < doc[top[i]].curr_t && p2 < query[q].curr_t) {// if overload break, else find same term
		// 			if(doc[top[i]].term[p1].voc1 > query[q].term[p2].voc1)
		// 				p2++;
		// 			else if(doc[top[i]].term[p1].voc1 < query[q].term[p2].voc1)
		// 				p1++;
		// 			else {
		// 				if(doc[top[i]].term[p1].voc2 > query[q].term[p2].voc2)
		// 					p2++;
		// 				else if(doc[top[i]].term[p1].voc2 < query[q].term[p2].voc2)
		// 					p1++;
		// 				else if(doc[top[i]].term[p1].voc2==-1 && query[q].term[p2].voc2==-1) { //only consider bi-nary
		// 					p1++;
		// 					p2++;
		// 				}
		// 				else {
		// 					double w = 1;
		// 					if(hum_f_flag) {
		// 						if(hum_f[q][i]<0)
		// 							w = hum_f[q][i]*750;
		// 						else
		// 							w = hum_f[q][i]*1500+1;
		// 					}
		// 					query[q].term[p2].rele_count+=doc[top[i]].term[p1].weight*w;
		// 					p1++;
		// 					p2++;
		// 				}
		// 			}
		// 		}
		// 	}
		// 	for(i=0;i<DOC_NUM;i++) {
		// 		p1=0;
		// 		p2=0;
		// 		doc[i].similarity = 0;
		// 		doc[i].cosinSp1 = 0;
		// 		doc[i].cosinSp2 = 0;
		// 		doc[i].check = 0; //in order to be faster in ranking
		// 		while(p1 < doc[i].curr_t && p2 < query[q].curr_t) {// if overload break, else find same term
		// 			if(doc[i].term[p1].voc1 > query[q].term[p2].voc1)
		// 				p2++;
		// 			else if(doc[i].term[p1].voc1 < query[q].term[p2].voc1)
		// 				p1++;
		// 			else {
		// 				if(doc[i].term[p1].voc2 > query[q].term[p2].voc2)
		// 					p2++;
		// 				else if(doc[i].term[p1].voc2 < query[q].term[p2].voc2)
		// 					p1++;
		// 				else if(doc[i].term[p1].voc2==-1 && query[q].term[p2].voc2==-1) { //only consider bi-nary
		// 					p1++;
		// 					p2++;
		// 				}
		// 				else {
		// 					doc[i].similarity += doc[i].term[p1].weight * (0.86 * query[q].term[p2].tf + alpha * query[q].term[p2].rele_count/R);
		// 					p1++;
		// 					p2++;
		// 				}
		// 			}
		// 		}
		// 		p1 = 0;
		// 		p2 = 0;
		// 		while(p1 < doc[i].curr_t) {
		// 			if(doc[i].term[p1].voc2!=-1) {
		// 				doc[i].cosinSp1 += doc[i].term[p1].weight*doc[i].term[p1].weight;
		// 			}
		// 			p1++;
		// 		}
		// 		while(p2 < query[q].curr_t) {
		// 			if(query[q].term[p2].voc2!=-1) {
		// 				doc[i].cosinSp2 += query[q].term[p2].tf*query[q].term[p2].tf;
		// 			}
		// 			p2++;
		// 		}
		// 		if(doc[i].cosinSp1!=0 && doc[i].cosinSp2!=0)
		// 			doc[i].cosine = doc[i].similarity/(sqrt(doc[i].cosinSp1)*sqrt(doc[i].cosinSp2)); // cosine similarity
		// 		else
		// 			doc[i].cosine = 0;
		// 	}
		// 	// generate top 100 anwser for each query//
		// 	for(i=0;i<100;i++) {
		// 		Maxsimi = 0;
		// 		for(j=0;j<DOC_NUM;j++) {
		// 			if(Maxsimi<doc[j].cosine && doc[j].check==0) {
		// 				Maxsimi = doc[j].cosine;
		// 				top[i] = j;
		// 			}
		// 		}
		// 		doc[top[i]].check = 1; // find top i and set check bit to 1, to exclude it.
		// 		//if(i>0 && wcscmp(doc[top[i]].doc_ID,doc[top[i-1]].doc_ID)==0)
		// 		//	i--;
		// 	}
		// }
		printf("query: %d\n", q);
		for (i = 0; i < 100; i++) {
			fwprintf(output_fp,L"%ls ",&query[q].doc_ID[14]);
			//wprintf(L"%d, %ls similarity:%lf,cosine:%lf\n",i+1,doc[top[i]].doc_ID,doc[top[i]].similarity,doc[top[i]].cosine);
			for (j = 0; j < wcslen(doc[top[i]].doc_ID); j++) {
				doc[top[i]].doc_ID[j] = towlower(doc[top[i]].doc_ID[j]);
			}
			fwprintf(output_fp, L"%ls\n", &doc[top[i]].doc_ID[16]);
			//fprintf(output_fp, "%s\n", &doc[top[i]].doc_ID[16]);
		}
		//printf("\n");
	}
	fclose(output_fp);

	return 0;
} 