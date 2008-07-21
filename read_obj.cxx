#include "contour_statistics.h"
#define BUFLEN 2048


void read_obj(FILE* mesh,SURFACE* surface){
	
	char line[BUFLEN];
	//char index;
	char dumm[128];
	float x=0;
	float y=0;
	float z=0;
	int first=0;
	int second=0;
	int third=0;

	VERTICES_LIST* vertices=(VERTICES_LIST*)malloc(sizeof(VERTICES_LIST));
	TRIANGLE_LIST* triangles=(TRIANGLE_LIST*)malloc(sizeof(TRIANGLE_LIST));

	memset(vertices,0,sizeof(VERTICES_LIST));
	memset(triangles,0,sizeof(TRIANGLE_LIST));
	vertices->num_vertices=0;
	triangles->num_triangles=0;

	vertices->x=(float*)malloc(sizeof(float));
	memset(vertices->x,0,sizeof(float));
	vertices->y=(float*)malloc(sizeof(float));
	memset(vertices->y,0,sizeof(float));
	vertices->z=(float*)malloc(sizeof(float));
	memset(vertices->z,0,sizeof(float));

	triangles->first=(int*)malloc(sizeof(int));
	memset(triangles->first,0,sizeof(int));
	triangles->second=(int*)malloc(sizeof(int));
	memset(triangles->second,0,sizeof(int));
	triangles->third=(int*)malloc(sizeof(int));
	memset(triangles->third,0,sizeof(int));

	//surface->vertices=(VERTICES_LIST)malloc(sizeof(VERTICES_LIST));
	//surface->triangles=(TRIANGLE_LIST)malloc(sizeof(TRIANGLE_LIST));

	if(!fgets(line,BUFLEN,mesh)){
		fprintf(stderr,"Error while parsing the file occurred, couldn't read the first line\n");
		exit(-1);
	}else if(sscanf(line,"g %s",&dumm)!=1){
		fprintf(stderr,"Error while parsing the file occurred, the first line in the .obj isn't in the correct format\n");
		exit(-1);
	}else{
		vertices=&surface->vertices;
		triangles=&surface->triangles;
	}

	while(fgets(line,BUFLEN,mesh)){
			
		if(sscanf(line,"v %f %f %f",&x,&y,&z)==3){
			vertices->num_vertices++;
			vertices->x=(float*)realloc(vertices->x,
						      (vertices->num_vertices)*sizeof(float));
			vertices->y=(float*)realloc(vertices->y,
						      (vertices->num_vertices)*sizeof(float));
			vertices->z=(float*)realloc(vertices->z,
						      (vertices->num_vertices)*sizeof(float));
			//printf("realloc riuscito, ho %d vertici\n",vertices->num_vertices);
			vertices->x[vertices->num_vertices-1]=x;
			vertices->y[vertices->num_vertices-1]=y;
			vertices->z[vertices->num_vertices-1]=z;
		/*	printf("I read: %f %f %f\n", vertices->x[vertices->num_vertices-1],
					vertices->y[vertices->num_vertices-1],vertices->z[vertices->num_vertices-1]);*/
		
		}else if(strstr(line,"#")!=NULL){
			printf("Loading the triangles\n");
		}else if(sscanf(line,"f %d %d %d",&first,&second,&third)==3){
			triangles->num_triangles++;
			triangles->first=(int*)realloc(triangles->first,
							(triangles->num_triangles)*sizeof(int));
			triangles->second=(int*)realloc(triangles->second,
							(triangles->num_triangles)*sizeof(int));
			triangles->third=(int*)realloc(triangles->third,
							(triangles->num_triangles)*sizeof(int));
			triangles->first[triangles->num_triangles-1]=first;
			triangles->second[triangles->num_triangles-1]=second;
			triangles->third[triangles->num_triangles-1]=third;
		}else{
			fprintf(stderr,"This is not the correct file format!");
			exit(-1);
		}
	}
	/*printf("LAST: %d %d %d\n",triangles->first[triangles->num_triangles-1],
		triangles->second[triangles->num_triangles-1], triangles->third[triangles->num_triangles-1]);*/
	fclose(mesh);
	}

void read_MDcontours(FILE* MDpoints, SURFACE* surface){

	char line[BUFLEN];
	float x=0;
	float y=0;
	float z=0;

	VERTICES_LIST* pointsMD=(VERTICES_LIST*)malloc(sizeof(VERTICES_LIST));
	memset(pointsMD,0,sizeof(VERTICES_LIST));
	pointsMD->num_vertices=0;

	pointsMD->x=(float*)malloc(sizeof(float));
	memset(pointsMD->x,0,sizeof(float));
	pointsMD->y=(float*)malloc(sizeof(float));
	memset(pointsMD->y,0,sizeof(float));
	pointsMD->z=(float*)malloc(sizeof(float));
	memset(pointsMD->z,0,sizeof(float));

	if(!fgets(line,BUFLEN,MDpoints)){
		fprintf(stderr,"Error while parsing the file occurred, couldn't read the first line\n");
		exit(-1);
	}else  if(strstr(line,"NaN NaN NaN")==NULL){
		fprintf(stderr,"Error while parsing MDpoints file occurred, the first line isn't in the correct format\n");
		exit(-1);
	}else{
		pointsMD=&surface->MDpoints;
	}

	while(fgets(line,BUFLEN,MDpoints)){
		//fprintf(stderr,"Error while parsing the file occurred, couldn't read something\n");
		//exit(-1);
		//}
		//fgets(line,BUFLEN,MDpoints);
		if(strstr(line,"NaN NaN NaN")!=NULL){
		}else if(sscanf(line,"%f %f %f",&x,&y,&z)==3){
			pointsMD->num_vertices++;
			pointsMD->x=(float*)realloc(pointsMD->x,
						      (pointsMD->num_vertices)*sizeof(float));
			pointsMD->y=(float*)realloc(pointsMD->y,
						      (pointsMD->num_vertices)*sizeof(float));
			pointsMD->z=(float*)realloc(pointsMD->z,
						      (pointsMD->num_vertices)*sizeof(float));
			//printf("realloc riuscito, ho %d vertici\n",pointsMD->num_vertices);
			pointsMD->x[pointsMD->num_vertices-1]=x;
			pointsMD->y[pointsMD->num_vertices-1]=y;
			pointsMD->z[pointsMD->num_vertices-1]=z;
		//printf("I read: %f %f %f\n", pointsMD->x[pointsMD->num_vertices-1],
		//			pointsMD->y[pointsMD->num_vertices-1],pointsMD->z[pointsMD->num_vertices-1]);
		}else
			fprintf(stderr,"Couldn't read the MDfile for unknown reasons\n");
	}
	fclose(MDpoints);
}