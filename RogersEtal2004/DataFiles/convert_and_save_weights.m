% A script to convert weights from PDPtool in a format readable by RC's hub
% implementation

% Load a saved state and then run this script.

filename = 'p1_4.wgt';
training_set = 'p1';
learning_rate = 0.001;
weight_decay = 0.001 / (48 * 3);
momentum = 0.0;
error_function = 'Cross entropy';
update_time = 'By epoch';
epochs = 1000;

% wstruct(1): 40 x 1: bias weights to Name output
% wstruct(2): 40 x 64: Hidden to Name output
% wstruct(3): 112 x 1: bias weights to Verbal output
% wstruct(4): 112 x 64: Hidden to Verbal output
% wstruct(5): 64 x 1: bias weights on Visual output
% wstruct(6): 64 x 64: Hidden to visual output
% wstruct(7): 64 x 1: bias weights on hidden units
% wstruct(8): 64 x 40: Name to Hidden
% wstruct(9): 64 X 112: Verbal to Hidden
% wstruct(10): 64 x 64: Visual to Hidden
% wstruct(11): 64 x 64: Hidden to Hidden

num_hidden = size(wtstruct(8).weight, 1);
num_name = size(wtstruct(1).weight, 1);
num_verbal = size(wtstruct(3).weight, 1);
num_visual = size(wtstruct(5).weight, 1);

fp = fopen(filename, 'w');
fprintf(fp, '%% Training set: %s\n', training_set);
fprintf(fp, '%% Learning rate: %f\n', learning_rate);
fprintf(fp, '%% Weight decay: %f\n', weight_decay);
fprintf(fp, '%% Momentum: %f\n', momentum);
fprintf(fp, '%% Update time: %s\n', update_time);
fprintf(fp, '%% Error function: %s\n', error_function);
fprintf(fp, '%% Epochs: %d\n', epochs);

% All inputs to hidden, including biases on hidden units
fprintf(fp, '%% %dI > %dH\n', num_name+num_verbal+num_visual, num_hidden);
for i = 1 : num_name
    for j = 1 : num_hidden
        fprintf(fp, '\t%f', wtstruct(8).weight(j, i));
    end
    fprintf(fp, '\n'); 
end
for i = 1 : num_verbal
    for j = 1 : num_hidden
        fprintf(fp, '\t%f', wtstruct(9).weight(j, i));
    end
    fprintf(fp, '\n');
end
for i = 1 : num_visual
    for j = 1 : num_hidden
        fprintf(fp, '\t%f', wtstruct(10).weight(j, i));
    end
    fprintf(fp, '\n');
end
% Bias weights to hidden units
for j = 1 : num_hidden
    fprintf(fp, '\t%f', wtstruct(7).weight(j, 1));
end
fprintf(fp, '\n');

% Hidden to hidden (no biases)
fprintf(fp, '%% %dH > %dH\n', num_hidden, num_hidden);
for i = 1 : num_hidden
    for j = 1 : num_hidden
        % Check: indicies might be wrong way around
        fprintf(fp, '\t%f', wtstruct(11).weight(j, i));
    end
    fprintf(fp, '\n');
end

% Hidden to all outputs to hidden, including biases on output units
fprintf(fp, '%% %dH > %dO\n', num_hidden, num_name+num_verbal+num_visual);

% wstruct(1): 40 x 1: bias weights to Name output
% wstruct(2): 40 x 64: Hidden to Name output
% wstruct(3): 112 x 1: bias weights to Verbal output
% wstruct(4): 112 x 64: Hidden to Verbal output
% wstruct(5): 64 x 1: bias weights on Visual output
% wstruct(6): 64 x 64: Hidden to visual output

for i = 1 : num_hidden
    for j = 1 : num_name
        fprintf(fp, '\t%f', wtstruct(2).weight(j, i));
    end
    for j = 1: num_verbal
        fprintf(fp, '\t%f', wtstruct(4).weight(j, i));
    end
    for j = 1: num_visual
        fprintf(fp, '\t%f', wtstruct(6).weight(j, i));
    end
    fprintf(fp, '\n');
end
% Bias weights to output units
for j = 1 : num_name
    fprintf(fp, '\t%f', wtstruct(1).weight(j, 1));
end
for j = 1: num_verbal
    fprintf(fp, '\t%f', wtstruct(3).weight(j, 1));
end
for j = 1: num_visual
    fprintf(fp, '\t%f', wtstruct(5).weight(j, 1));
end
fprintf(fp, '\n');

fclose(fp);
