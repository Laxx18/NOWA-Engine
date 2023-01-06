/* Copyright (c) <2003-2016> <Newton Game Dynamics>
* 
* This software is provided 'as-is', without any express or implied
* warranty. In no event will the authors be held liable for any damages
* arising from the use of this software.
* 
* Permission is granted to anyone to use this software for any purpose,
* including commercial applications, and to alter it and redistribute it
* freely
*/

#include "dAnimationStdAfx.h"
#include "dAnimationPose.h"
#include "dAnimationSequence.h"

dAnimationSequence::dAnimationSequence()
	:m_tracks()
	,m_period(1.0f)
{
}

dAnimationSequence::~dAnimationSequence()
{
}

dAnimimationKeyFramesTrack* dAnimationSequence::AddTrack()
{
	dList<dAnimimationKeyFramesTrack>::dListNode* const node = m_tracks.Append();
	return &node->GetInfo();
}

void dAnimationSequence::CalculatePose(dAnimationPose& output, dFloat t) const
{
	int index = 0;
	dAnimKeyframe* const keyFrames = &output[0];
	for (dList<dAnimimationKeyFramesTrack>::dListNode* srcNode = m_tracks.GetFirst(); srcNode; srcNode = srcNode->GetNext()) {
		const dAnimimationKeyFramesTrack& track = srcNode->GetInfo();
		dAnimKeyframe& keyFrame = keyFrames[index];
		track.InterpolatePosition(t, keyFrame.m_posit);
		track.InterpolateRotation(t, keyFrame.m_rotation);
		dAssert(keyFrame.m_rotation.DotProduct(keyFrame.m_rotation) > 0.999f);
		dAssert(keyFrame.m_rotation.DotProduct(keyFrame.m_rotation) < 1.001f);

		index ++;
	}
}

void dAnimationSequence::Load(const char* const fileName)
{
	char* const oldloc = setlocale(LC_ALL, 0);
	setlocale(LC_ALL, "C");

	m_tracks.RemoveAll();
	m_period = 0;
	FILE* const file = fopen(fileName, "rb");
	if (file) {
		int tracksCount;
		fscanf(file, "period %f\n", &m_period);
		fscanf(file, "tracksCount %d\n", &tracksCount);
		for (int i = 0; i < tracksCount; i++) {
			dAnimimationKeyFramesTrack* const track = AddTrack();
			track->Load(file);
		}
		fclose(file);
	}

	// restore locale settings
	setlocale(LC_ALL, oldloc);
}

void dAnimationSequence::Save(const char* const fileName)
{
	char* const oldloc = setlocale(LC_ALL, 0);
	setlocale(LC_ALL, "C");

	FILE* const file = fopen(fileName, "wb");
	if (file) {
		fprintf(file, "period %f\n", m_period);
		fprintf(file, "tracksCount %d\n", m_tracks.GetCount());
		for (dList<dAnimimationKeyFramesTrack>::dListNode* trackNode = m_tracks.GetFirst(); trackNode; trackNode = trackNode->GetNext()) {
			dAnimimationKeyFramesTrack& track = trackNode->GetInfo();
			track.Save(file);
		}
		fclose(file);
	}

	// restore locale settings
	setlocale(LC_ALL, oldloc);
}
