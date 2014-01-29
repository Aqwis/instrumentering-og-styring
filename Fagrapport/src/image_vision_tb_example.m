FILENAME = 'face.jpg'

detector = vision.CascadeObjectDetector();
img = imread(FILENAME);
bIns = vision.ShapeInserter('BorderColor', 'Custom', 'CustomBorderColor', [255 255 0]);
bbox = step(det, img)

videoOut = step(bIns, img, uint32(bbox));
figure; imshow(videoOut);