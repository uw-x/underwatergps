
close all
% clc
clear 
fs=44100;
    
% PN_seq = [1    -1    -1    -1    -1    -1   1    -1];

c = 1500; %% sound speed underwater


BW = [1000, 5000];

sig_type = 6;


%% load the recving data to process in reply side
raw_folder = '../data_example/';
save_fig = 1;
exp_num = 1;
listings = dir(strcat(raw_folder, int2str(exp_num), '/')) ;

group_users = [];
roles = [];
for i = 1:size(listings, 1)
    fd = listings(i);
    if(strcmp(fd.name, 'L0'))
        group_users = [group_users, 0];
        roles = [roles; fd.name];
    elseif(strcmp(fd.name, 'U0'))
        group_users = [group_users, 1]; 
        roles = [roles; fd.name];
    elseif(strcmp(fd.name, 'U1'))
        group_users = [group_users, 2];
        roles = [roles; fd.name];
    elseif(strcmp(fd.name, 'U2'))
        group_users = [group_users, 3];
        roles = [roles; fd.name];
    elseif(strcmp(fd.name, 'U3'))
        group_users = [group_users, 4];
        roles = [roles; fd.name];
    elseif(strcmp(fd.name, 'U4'))
        group_users = [group_users, 5];
        roles = [roles; fd.name];
    end
end

% group_users = [0, 1, 2, 3, 4];% configure 1
% group_users = [0,  2]; %%config2
group_size = length(group_users);

pos_gt = zeros(group_size, 3);
DIS_gt = zeros(group_size);

%% Load the raw sending signals from the raw folder
% Raw dataset 

PN_seq = [1, 1, -1, 1];
L = length(PN_seq);
Ns = 1920; %720
GI = 540; %360
N_FSK = 2720;
sig_len1 = Ns;
sig_len2 = (Ns + GI)*L;
file_id = '6';
save_file = strcat('N_seq5-', int2str(0), '-', int2str(Ns), '-', int2str(GI), '-', int2str(BW(1)), '-', int2str(BW(2)));
train_sig1s = zeros(sig_len1, group_size);
train_sig2s = zeros(sig_len2, group_size);
train_idx = zeros(16, 6);

for i =0:5
    save_file = strcat('N_seq5-', int2str(i*2), '-', int2str(Ns), '-', int2str(GI), '-', int2str(BW(1)), '-', int2str(BW(2)));
    train_sig1s(:, i+1) = dlmread(strcat('multi_users_FSK3/',save_file,'/train_sig1.txt')); % this is the signal for single OFDM symbol
    long_sig = dlmread(strcat('multi_users_FSK3/',save_file,'/train_sig2.txt')); % this is the signal for the entitr preamble contains 4 OFDM symbols
    train_sig2s(:, i+1) = long_sig(1:sig_len2); 
    train_idx(:, i+1) = dlmread(strcat('multi_users_FSK3/',save_file,'/FSK_IDX.txt')); % this is the FSK codes to decode the user id of each packets.
end

color_lists = [	'r', 'g', 	'b', 'c', 'm', 'y', 'k' ];

extracted_index = zeros(group_size, 20, group_size);
valid_meas = ones(20, 1);
T_min = 9999;
% get the ground-truth pairwise range
DIS_gt = zeros(group_size, group_size);
pos_gt = dlmread(strcat(raw_folder, 'gt.txt'));

for i =1:group_size
    for j = i+1:group_size
        tmp = sqrt(sumsqr(pos_gt(i, :) - pos_gt(j, :)));
        DIS_gt(i, j) = tmp;
        DIS_gt(j, i) = tmp;
    end
end
DIS_gt
figure
plot(long_sig)

for r =1:(group_size)
    USER_ID = group_users(r);
    role = roles(r, :);
    exp = ' ';
    folders = strcat(raw_folder, int2str(exp_num), '/', role, '/');
    filenames = dir(folders);
    for j = 1:size(filenames, 1)
        name = filenames(j).name;
        if contains(name, '16')
            exp = name;
            break;
        end
    end

    %%% load the raw recoreded audio underwater
    rx_signal = dlmread(strcat(folders,  exp, '/', exp, '-',file_id,'-bottom.txt'))/10000; % bottom mic channel 
    rx_signal2 = dlmread(strcat(folders, exp, '/', exp, '-',file_id,'-top.txt'))/10000; % top mic channel 

    %%% the beginning 32s of leader during experiment is sleeping
    if role ~= 'L0'
        rx_signal = rx_signal(32*fs+1:end);
        rx_signal2 = rx_signal2(32*fs+1:end); 
    end
   
    %% apply the coarse and fine-grained signal arrival detection 
    [fine_result, coarse_result, rx_signal, rx_signal2] = fine_sync_recv_single(rx_signal,rx_signal2,train_sig1s, train_sig2s, fs, BW, Ns, GI, L, PN_seq, c, USER_ID, group_users, train_idx, save_fig, N_FSK);
    
    %% visualize the signal detection results
    figure
    hold on
    plot(rx_signal)
    plot(rx_signal2, '--')
    for i = 1:size(fine_result, 1)
        for j = 1:size(fine_result, 2)
            xline(fine_result(i, j)+270,  '--', 'Color',color_lists(j))
        end
    end
    title('signal detection synchronization')


    % matching the pairwise ranging between different phones, this is only
    % needed for offline, for online it does not match
    T = size(fine_result, 1);
    for t = 1:T
        if min(fine_result(t, :), [], 'all') < 0
            valid_meas(t, 1) = 0;
        end
    end
    extracted_index(r, 1:T, :) =  fine_result(:, :) - fine_result(:, 1);
    if(T < T_min)
        T_min = T;
    end

end

%%%%%% process the distance matrix
for i = 1:T_min
    if valid_meas(i, 1) == 0
        continue
    end
    dis_matrix = zeros(group_size);
    meas = squeeze(extracted_index(:, i, :));
    meas = meas(:, 2:end) - meas(:, 1:end-1);
    for k = 1:group_size
        for j = k+1:group_size
            d = ( sum(meas(k,  k:j-1)) - sum(meas(j, k:j-1)) )/2/fs*c;
            dis_matrix(k, j) = d;
            dis_matrix(j, k) = d;
        end
    end
    if min(dis_matrix, [], 'all') >= 0 
        dis_matrix
        dlmwrite(strcat(raw_folder, int2str(exp_num), '/result', int2str(i), '.txt'), dis_matrix );
    end
    
end



