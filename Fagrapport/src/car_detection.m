% Detekterer punkter i bildet som er fornuftige å tracke

hold off;

im = imread('motorway.jpg');
gr = cv.cvtColor(im, 'RGB2GRAY');
gr = cv.equalizeHist(gr);

corners = cv.goodFeaturesToTrack(gr);
corners_m = cell2mat(corners');
corners_m = corners_m';

number_of_points = size(test2, 2)
kept_points = floor(number_of_points/4)

imshow(im);
hold on;
plot(corners_m(1,1:kept_points), corners_m(2,1:kept_points), 'go');
