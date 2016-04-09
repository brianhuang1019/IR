#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <wctype.h>
#include <math.h>
//#include <ctype.h>
#include <locale.h>

#define DOCUMENT_NUM 50000
#define VOCAB_NUM 30000
#define DOC_NUM 46972
#define VOC_NUM 29908

// tf-IDF
#define SMOOTH 1
#define k 2.7
#define b 0.985

// Rocchio relevance feedback
#define RELE_DOC_NUM 7
#define ALPHA 0.9
#define BETA 0.021

// k = 2.23, b = 1.02, no Roccico, 0.795256298517
// k = 2.23, b = 1.01, no Roccico, 0.795294513996
// k = 2.23, b = 1.00, no Roccico, 0.7953
// k = 2.23, b = 0.99, no Roccico, 0.7960
// k = 2.23, b = 0.98, no Roccico, 0.795663904799
// k = 2.23, b = 0.97, no Roccico, 0.795572972343
// k = 2.23, b = 0.96, no Roccico, 0.795686692052
// k = 2.25, b = 1.02, no Roccico, 0.795209833666
// k = 2.25, b = 1.01, no Roccico, 0.795303833536

// k = 2.75, b = 1.00, no Roccico, 0.796720249565
	// fix tf's bugs, sort document's vocabs' ID, plus unigram, cosine include unigram, 0.807705843958
	// fix tf's bugs, sort document's vocabs' ID, plus unigram, 0.807594902977
	// fix tf's bugs, sort document's vocabs' ID, no unigram, 0.803256077045

// fix tf's bugs, sort document's vocabs' ID, plus unigram, cosine include unigram, with Rocchio, 0.807917128127
	// BETA = 0.0253
	// k = 2.75, b = 1.00, ALPHA = 0.86, 0.800086942691
	// k = 2.75, b = 0.99, ALPHA = 0.86, 0.808238386901
	// k = 2.75, b = 0.99, ALPHA = 0.88, 0.808470880362
	// k = 2.75, b = 0.99, ALPHA = 0.89, 0.808471998523
	// k = 2.75, b = 0.99, ALPHA = 0.895, 0.808471998523
	// k = 2.75, b = 0.99, ALPHA = 0.90, 0.808471998523
	// k = 2.75, b = 0.985, ALPHA = 0.90, 0.808482874806
	// k = 2.8, b = 0.985, ALPHA = 0.90, 0.807052254986
	// k = 2.7, b = 0.985, ALPHA = 0.90, 0.808021020087
	// k = 2.75, b = 0.985, ALPHA = 0.90, BETA = 0.022, 0.808212078565
	// k = 2.75, b = 0.985, ALPHA = 0.90, BETA = 0.0215, 0.808261266354
	// k = 2.75, b = 0.985, ALPHA = 0.90, BETA = 0.02125, 0.808874665902
	// k = 2.75, b = 0.985, ALPHA = 0.90, BETA = 0.021, 0.808929322404
	// k = 2.75, b = 0.985, ALPHA = 0.90, BETA = 0.0205, 0.808731189251
	// k = 2.75, b = 0.985, ALPHA = 0.90, BETA = 0.02, 0.808801022588
	// k = 2.75, b = 0.985, ALPHA = 0.90, BETA = 0.018, 0.808738330526

// RELE_DOC_NUM = 2, k = 2.75, b = 0.985, ALPHA = 0.90, BETA = 0.021, 0.808929322404
	// RELE_DOC_NUM = 3, 0.810780039908
	// RELE_DOC_NUM = 4, 0.81159137397
	// RELE_DOC_NUM = 5, 0.810696393829
	// RELE_DOC_NUM = 6, 0.813287916364
	// RELE_DOC_NUM = 7, 0.813607310423
	// RELE_DOC_NUM = 8, 0.812473928664
	// RELE_DOC_NUM = 9, 0.811318102076
	// RELE_DOC_NUM = 15, 0.811321134563
	// RELE_DOC_NUM = 100, 0.80664257293

typedef struct {
	int vocab1;				// first vocabulary
	int vocab2;				// second vocabulary
	double tf;				// term frequency in this doc
	double tf_normalize;	// Okapi / BM25
	double df; 				// document frequence of the term
	double weight;			// tf-IDF
	double relevance_doc_vector; 		// use to calculate the relevance in relevance feedback 
} _Term;

typedef struct {
	wchar_t doc_ID[100];
	int max_term;
	int current_term;
	_Term* term;
	double similarity;		// dot product of doc & query
	double cosine_amount1;
	double cosine_amount2;
	double cosine;
	int dirty_bit;
} _Document;

wchar_t vocab[VOCAB_NUM][100]; 		// total vocab number with length maximum 100
_Document doc[DOCUMENT_NUM]; 		// total document number
_Document *query; 					// query document

int compare (const void *a1, const void *a2) {
	_Term *term1 = (_Term*)a1;
	_Term *term2 = (_Term*)a2;
	if (term1->vocab1 < term2->vocab1) 
		return -1;
	else if (term1->vocab1 > term2->vocab1)
		return 1;
	else {
		if (term1->vocab2 < term2->vocab2)
			return -1;
		else
			return 1;
	}
}

double count_IDF(double doc_frequency) {
	return log((double)(DOC_NUM + 0.5) / (doc_frequency + 0.5));
}

double count_TF(double term_frequency, int current_term, double avgdoclen) {
	return (k + 1) * term_frequency / (term_frequency + k * (1 - b + b * current_term / avgdoclen));
}

double count_similarity(double doc_term_weight, double query_term_weight) {
	return doc_term_weight * query_term_weight * SMOOTH;
}

double count_relevance_feedback_similarity(double doc_term_weight, double query_term_weight, double query_term_relevance) {
	return doc_term_weight * (ALPHA * query_term_weight + BETA * query_term_relevance / RELE_DOC_NUM);
}

double count_cosine(double similarity, double cos1, double cos2) {
	return similarity / (sqrt(cos1) * sqrt(cos2));
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

	int default_query_num = 30;
	int relevance_feedback = 0;
	char query_file[50];
	char output_file[50] = "ranked-list";
	char model_dir[50];
	char NTCIR_dir[50];
	char tmp[100];

	FILE *query_file_fp,	// query file
		 *output_file_fp,	// output file 
		 *vocal_all_fp, 	// vocab.all
		 *file_list_fp, 	// file-list
		 *inverted_file_fp;	// inverted-file

	// process input parameters
	while (i < argc) {
		if(strcmp(argv[i], "-r") == 0) {
			relevance_feedback = 1; // open relevance feedback
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
		else {
			//something else
		}
		i++;
	}

	// open file pointers
	strcpy(tmp, "./");
	strcat(tmp, query_file);
	query_file_fp = fopen(tmp, "r");
	if (query_file_fp == NULL)
		printf("opening query-file error\n");

	strcpy(tmp, "./");
	strcat(tmp, output_file);
	output_file_fp = fopen(tmp, "w");
	if (output_file_fp == NULL)
		printf("creating ranked-list error\n");

	strcpy(tmp, model_dir);
	if (model_dir[strlen(model_dir)-1] != '/')
		strcat(tmp, "/");
	strcat(tmp, "vocab.all"); // only use when we check the query file
	vocal_all_fp = fopen(tmp, "r");
	if (vocal_all_fp == NULL)
		printf("opening vocab.all error\n");

	strcpy(tmp, model_dir);
	if (model_dir[strlen(model_dir)-1] != '/')
		strcat(tmp, "/");
	strcat(tmp, "file-list");
	file_list_fp = fopen(tmp, "r");
	if (file_list_fp == NULL)
		printf("opening file-list error\n");

	strcpy(tmp, model_dir);
	if (model_dir[strlen(model_dir)-1] != '/')
		strcat(tmp, "/");
	strcat(tmp, "inverted-file");
	inverted_file_fp = fopen(tmp, "r");
	if (inverted_file_fp == NULL) 
		printf("opening inverted-file error\n");

	// inital _Document and query space
	query = malloc(sizeof(_Document) * default_query_num);

	for (i = 0; i < DOC_NUM; i++) {
		doc[i].max_term = 5;
		doc[i].current_term = 0;
		doc[i].term = malloc(sizeof(_Term) * 5);
	}

	// parse vocab.all
	i = 0;
	while (fwscanf(vocal_all_fp, L"%ls", &vocab[i]) != EOF)
		i++;
	fclose(vocal_all_fp);
	printf(" [O] Parsing vocab.all finish\n");

	// parse file-list
	for (i = 0; i < DOC_NUM; i++) {
		fwscanf(file_list_fp, L"%ls", &doc[i].doc_ID);
	}
	fclose(file_list_fp);
	printf(" [O] Parsing file-list finish\n");

	// pasre XML query
	wchar_t c, cn;
	int query_index, query_num = 0;
	wchar_t narr_buffer[1000], query_buffer[200], tmp_buffer[1000];
	while ((c = fgetwc(query_file_fp)) != EOF) {
		if (c == L'<') {
			c = fgetwc(query_file_fp);
			if (c == L'n') {
				c = fgetwc(query_file_fp);
				if (c == L'u') {	// <number>
					query_num += 1;
					query_index = query_num - 1;
					if (query_num > default_query_num) {
						default_query_num *= 2;
						query = realloc(query, sizeof(_Document) * default_query_num);
					}
					query[query_index].max_term = 30000;
					query[query_index].current_term = 0;
					query[query_index].term = malloc(sizeof(_Term) * 30000);

					// parse query name
					wchar_t buffer[100];
					fwscanf(query_file_fp, L"%ls", &buffer);
					wchar_t *name_start = wcschr(buffer, L'>');
					name_start++;
					wchar_t *name_end = wcschr(buffer, L'<');
					*name_end = L'\0';
					wcscpy(query[query_index].doc_ID, name_start);
				}	
			}
			else if (c == L'q') {	// <question>
				c = fgetwc(query_file_fp);
				if (c == L'u') {
					while ((c = fgetwc(query_file_fp)) != L'\n');
					fwscanf(query_file_fp, L"%ls", &query_buffer);
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
						for (j = 0; j < VOC_NUM; j++) {
							if(vocab[j][0] == query_buffer[i+1])
								break;	
						}
						v2 = j;
						//wprintf(L"%lc", vocab[v1][0]);
						//wprintf(L"%lc\n", vocab[v2][0]);

						// find repeat _Term, avoid puting same term into query document
						for (j = 0; j < query[query_index].current_term; j++) {
							if (query[query_index].term[j].vocab1 == v1 && query[query_index].term[j].vocab2 == v2) {
								//printf(" [!] size: %d, j: %d\n", query[query_index].current_term, j);
								if (query[query_index].term[j].tf <= 0)
									query[query_index].term[j].tf = 1;
								else
									query[query_index].term[j].tf += 1;
								break;
							}
						}
						//printf("size: %d, j: %d\n", query[query_index].current_term, j);

						// if not repeat, put this term into set
						if (j == query[query_index].current_term) {
							query[query_index].term[query[query_index].current_term].vocab1 = v1;
							query[query_index].term[query[query_index].current_term].vocab2 = v2;
							query[query_index].term[query[query_index].current_term].tf = 1;
							query[query_index].current_term += 1;
						}
					}
					// for (i = 0; i < query[query_index].current_term; i++)
					// 	printf("i: %d, tf: %lf\n", i, query[query_index].term[i].tf);
				}
			}
			else if (c == L'c') {
				c = fgetwc(query_file_fp);
				if (c == L'o') {	// <concept>
					while ((c = fgetwc(query_file_fp)) != L'\n');
					while ((c = fgetwc(query_file_fp)) != L'<') {
						cn = fgetwc(query_file_fp);
						ungetwc(cn, query_file_fp);
						if (c == L'、' || c == L'。' || c == L'<' || cn == L'<')
							continue;

						int v1, v2;
						for (i = 0; i < VOC_NUM; i++)
							if (vocab[i][0] == c)
								break;
						v1 = i;

						for (i = 0; i < VOC_NUM; i++)
							if (vocab[i][0] == cn)
								break;

						if (cn == L'、' || cn == L'。')
							v2 = -1;
						else
							v2 = i;

						// find repeat _Term, avoid puting same term into query document
						for (i = 0; i < query[query_index].current_term; i++) {
							if (query[query_index].term[i].vocab1 == v1 && query[query_index].term[i].vocab2 == v2) {
								if(query[query_index].term[i].tf<=0)
									query[query_index].term[i].tf = 1;
								else
									query[query_index].term[i].tf += 1;
								break;
							}
						}

						// if not repeat, put this term into set
						if (i == query[query_index].current_term) {
							query[query_index].term[query[query_index].current_term].vocab1 = v1;
							query[query_index].term[query[query_index].current_term].vocab2 = v2;
							query[query_index].term[query[query_index].current_term].tf = 1;
							query[query_index].term[query[query_index].current_term].relevance_doc_vector = 0;
							query[query_index].current_term += 1;
							//printf("%d %d\n",v1,v2);
						}
						//wprintf(L"%lc %lc %d %d\n",c,cn,v1,v2);
					}
				}
			}
		}
	}
	fclose(query_file_fp);

	// sort _Term in query by voc ID
	for (i = 0; i < query_num; i++) {
		qsort(query[i].term, query[i].current_term, sizeof(_Term), compare);
	}
	printf(" [O] Parsing query-file finish\n");

	// parse inverted-file
	int vocab1, vocab2, docnum;
	int doc_id, frequency;
	int cur;
	while (fscanf(inverted_file_fp, "%d %d %d", &vocab1, &vocab2, &docnum) != EOF) {
		for (i = 0; i < docnum; i++) {
			fscanf(inverted_file_fp, "%d %d", &doc_id, &frequency);
			doc[doc_id].current_term += 1;
			if (doc[doc_id].current_term > doc[doc_id].max_term) {	// space not enough
				doc[doc_id].max_term *= 2; 
				doc[doc_id].term = realloc(doc[doc_id].term, sizeof(_Term) * doc[doc_id].max_term);
			}
			cur = doc[doc_id].current_term - 1;
			doc[doc_id].term[cur].vocab1 = vocab1;
			doc[doc_id].term[cur].vocab2 = vocab2;
			doc[doc_id].term[cur].tf = frequency;	// term frequence in this doc
			doc[doc_id].term[cur].df = docnum; 		// doc frequence
		}
	}
	fclose(inverted_file_fp);
	printf(" [O] Parsing inverted-file finish\n");

	// calculate average document length
	double avgdoclen = 0;
	for (i = 0; i < DOC_NUM; i++) {
		avgdoclen += doc[i].current_term;
	}
	avgdoclen = avgdoclen / DOC_NUM;
	printf(" [O] Calculate avgdoclen finish, it equals to: %lf\n", avgdoclen);

	// calculate term weight (using tf-IDF, tf with Okapi/BM25)
	for (i = 0; i < DOC_NUM; i++) {
		for (j = 0; j < doc[i].current_term; j++) {
			doc[i].term[j].tf_normalize = count_TF(doc[i].term[j].tf, doc[i].current_term, avgdoclen);
			doc[i].term[j].weight = count_IDF(doc[i].term[j].df) * doc[i].term[j].tf_normalize;
		}
	}
	// sort _Term in doc by voc ID
	for (i = 0; i < DOC_NUM; i++) {
		qsort(doc[i].term, doc[i].current_term, sizeof(_Term), compare);
	}
	printf(" [O] Calculate term weight using tf-IDF finish\n");

	// calculate similarity with each query file//
	int q;
	for (q = 0; q < query_num; q++) {
		// use method in HW1 to find same vocab
		int p1, p2;
		for (i = 0; i < DOC_NUM; i++) {
			p1 = 0;
			p2 = 0;
			doc[i].similarity = 0;
			doc[i].cosine_amount1 = 0;
			doc[i].cosine_amount2 = 0;
			doc[i].dirty_bit = 0;	// in order to be faster in ranking
			while (p1 < doc[i].current_term && p2 < query[q].current_term) {
				if (doc[i].term[p1].vocab1 > query[q].term[p2].vocab1)
					p2++;
				else if (doc[i].term[p1].vocab1 < query[q].term[p2].vocab1)
					p1++;
				else {
					if (doc[i].term[p1].vocab2 > query[q].term[p2].vocab2)
						p2++;
					else if (doc[i].term[p1].vocab2 < query[q].term[p2].vocab2)
						p1++;
					// else if (doc[i].term[p1].vocab2 == -1 && query[q].term[p2].vocab2 == -1) {	// unigram 0.79
					// 	p1++;
					// 	p2++;
					// }
					else {
						doc[i].similarity += count_similarity(doc[i].term[p1].weight, query[q].term[p2].tf);
						//doc[i].similarity += (doc[i].term[p1].weight * query[q].term[p2].tf * SMOOTH);
						//doc[i].similarity += (doc[i].term[p1].weight * query[q].term[p2].weight * SMOOTH);
						p1++;
						p2++;
					}
				}
			}
			p1 = 0;
			p2 = 0;
			while (p1 < doc[i].current_term) {
				if (doc[i].term[p1].vocab2 != -1) {	// no unigram
					doc[i].cosine_amount1 += doc[i].term[p1].weight * doc[i].term[p1].weight;
				}
				p1++;
			}
			while (p2 < query[q].current_term) {
				if (query[q].term[p2].vocab2 != -1) {	// no unigram
					doc[i].cosine_amount2 += query[q].term[p2].tf * query[q].term[p2].tf;
				}
				p2++;
			}
			if (doc[i].cosine_amount1 != 0 && doc[i].cosine_amount2 != 0)
				doc[i].cosine = count_cosine(doc[i].similarity, doc[i].cosine_amount1, doc[i].cosine_amount2);
			else
				doc[i].cosine = 0;
		}

		// generate top 100 anwser for each query//
		int top[100];
		double maximal;
		for (i = 0; i < 100; i++) {
			maximal = 0;
			for (j = 0; j < DOC_NUM; j++) {
				if (maximal < doc[j].cosine && doc[j].dirty_bit == 0) {
					maximal = doc[j].cosine;
					top[i] = j;
				}
			}
			// set dirty bit
			doc[top[i]].dirty_bit = 1;
		}

		//if relevance feeback is specified
		if (relevance_feedback) {
			int p1, p2;
			for (i = 0; i < RELE_DOC_NUM; i++) {
				p1 = 0;
				p2 = 0;
				// find same terms that appear both in query and its top <RELE_DOC_NUM> documents
				while (p1 < doc[top[i]].current_term && p2 < query[q].current_term) {
					if (doc[top[i]].term[p1].vocab1 > query[q].term[p2].vocab1)
						p2++;
					else if (doc[top[i]].term[p1].vocab1 < query[q].term[p2].vocab1)
						p1++;
					else {
						if (doc[top[i]].term[p1].vocab2 > query[q].term[p2].vocab2)
							p2++;
						else if (doc[top[i]].term[p1].vocab2 < query[q].term[p2].vocab2)
							p1++;
						// else if (doc[top[i]].term[p1].vocab2 == -1 && query[q].term[p2].vocab2 == -1) {	// no unigram
						// 	p1++;
						// 	p2++;
						// }
						else {
							double w = 1;
							// good term, add its weight to relevance
							query[q].term[p2].relevance_doc_vector += doc[top[i]].term[p1].weight * w;
							//query[q].term[p2].relevance_doc_vector += query[q].term[p2].tf * w;
							p1++;
							p2++;
						}
					}
				}
			}
			for (i = 0; i < DOC_NUM; i++) {
				p1 = 0;
				p2 = 0;
				doc[i].similarity = 0;
				doc[i].cosine_amount1 = 0;
				doc[i].cosine_amount2 = 0;
				doc[i].dirty_bit = 0;
				while (p1 < doc[i].current_term && p2 < query[q].current_term) {
					if (doc[i].term[p1].vocab1 > query[q].term[p2].vocab1)
						p2++;
					else if (doc[i].term[p1].vocab1 < query[q].term[p2].vocab1)
						p1++;
					else {
						if (doc[i].term[p1].vocab2 > query[q].term[p2].vocab2)
							p2++;
						else if (doc[i].term[p1].vocab2 < query[q].term[p2].vocab2)
							p1++;
						// else if (doc[i].term[p1].vocab2 == -1 && query[q].term[p2].vocab2 == -1) {	// no unigram
						// 	p1++;
						// 	p2++;
						// }
						else {
							doc[i].similarity += count_relevance_feedback_similarity(doc[i].term[p1].weight, query[q].term[p2].tf, query[q].term[p2].relevance_doc_vector);
							p1++;
							p2++;
						}
					}
				}
				p1 = 0;
				p2 = 0;
				while (p1 < doc[i].current_term) {
					if (doc[i].term[p1].vocab2 != -1) {
						doc[i].cosine_amount1 += doc[i].term[p1].weight * doc[i].term[p1].weight;
					}
					p1++;
				}
				while (p2 < query[q].current_term) {
					if (query[q].term[p2].vocab2 != -1) {
						doc[i].cosine_amount2 += query[q].term[p2].tf * query[q].term[p2].tf;
					}
					p2++;
				}
				if (doc[i].cosine_amount1 != 0 && doc[i].cosine_amount2 != 0)
					doc[i].cosine = count_cosine(doc[i].similarity, doc[i].cosine_amount1, doc[i].cosine_amount2);
				else
					doc[i].cosine = 0;
			}

			// generate top 100 anwser for each query//
			for (i = 0; i < 100; i++) {
				maximal = 0;
				for (j = 0; j < DOC_NUM; j++) {
					if (maximal < doc[j].cosine && doc[j].dirty_bit == 0) {
						maximal = doc[j].cosine;
						top[i] = j;
					}
				}
				doc[top[i]].dirty_bit = 1;
			}
		}

		wprintf(L" query: %ls\n", query[q].doc_ID);
		// print top100 into output
		for (i = 0; i < 100; i++) {
			fwprintf(output_file_fp, L"%ls ", &query[q].doc_ID[14]);
			//wprintf(L"%d, %ls similarity:%lf,cosine:%lf\n",i+1,doc[top[i]].doc_ID,doc[top[i]].similarity,doc[top[i]].cosine);
			for (j = 0; j < wcslen(doc[top[i]].doc_ID); j++) {
				doc[top[i]].doc_ID[j] = towlower(doc[top[i]].doc_ID[j]);
			}
			fwprintf(output_file_fp, L"%ls\n", &doc[top[i]].doc_ID[16]);
		}
	}
	fclose(output_file_fp);
	printf("\r [O] Calculate similarity with relevance feedback\n");

	return 0;
} 