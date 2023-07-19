
close all
% clc
clear 
fs=44100;
    
% PN_seq = [1    -1    -1    -1    -1    -1   1    -1];

c = 1500;


BW = [1000, 5000];

sig_type = 6;

group_users = [0, 2, 3, 4];% configure 1
% group_users = [0,  2]; %%config2
group_size = length(group_users);

pos_gt = zeros(group_size, 3);
DIS_gt = zeros(group_size);
%%%% group 4


save_fig = 0;

if(sig_type == 7)
    PN_seq = [1, 1, 1, -1, 1, -1, -1];
    L = length(PN_seq);
    file_ids = [1, 11, 22, 35, 41, 76];
    Ns = 960; %720
    GI = 360; %360
    sig_len1 = Ns;
    sig_len2 = (Ns + GI)*L;
    train_sig1s = zeros(sig_len1, group_size);
    train_sig2s = zeros(sig_len2, group_size);
    
    for i =1:group_size
        save_file = strcat('N_seq7-', int2str(file_ids(i)), '-', int2str(Ns), '-', int2str(GI), '-', int2str(BW(1)), '-', int2str(BW(2)));
        train_sig1s(:, i) = dlmread(strcat('./multi-users-7/',save_file,'/train_sig1.txt'));
        train_sig2s(:, i) = dlmread(strcat('./multi-users-7/',save_file,'/train_sig2.txt'));
    end
elseif(sig_type == 5)
    PN_seq = [1, 1, -1, 1, -1];
    L = length(PN_seq);
    file_ids = [1, 5, 11, 22, 35, 49];
    Ns = 1450; %720
    GI = 480; %360
    sig_len1 = Ns;
    sig_len2 = (Ns + GI)*L;
    
    
    train_sig1s = zeros(sig_len1, group_size);
    train_sig2s = zeros(sig_len2, group_size);
    
    for i =1:group_size
        save_file = strcat('N_seq5-', int2str(file_ids(i)), '-', int2str(Ns), '-', int2str(GI), '-', int2str(BW(1)), '-', int2str(BW(2)));
        train_sig1s(:, i) = dlmread(strcat('./multi-users-5/',save_file,'/train_sig1.txt'));
        train_sig2s(:, i) = dlmread(strcat('./multi-users-5/',save_file,'/train_sig2.txt'));
    end
elseif(sig_type == 4)
    %%% FSK signal
    PN_seq = [1, 1, -1, 1, -1];
    L = length(PN_seq);
    Ns = 1450; %720
    GI = 480; %360
    sig_len1 = Ns;
    sig_len2 = (Ns + GI)*L;

    save_file = strcat('N_seq5-', int2str(0), '-', int2str(Ns), '-', int2str(GI), '-', int2str(BW(1)), '-', int2str(BW(2)));
    train_sig1s = zeros(sig_len1, group_size);
    train_sig2s = zeros(sig_len2, group_size);
    train_idx = zeros(15, 6);

    for i =0:group_size-1
        save_file = strcat('N_seq5-', int2str(i), '-', int2str(Ns), '-', int2str(GI), '-', int2str(BW(1)), '-', int2str(BW(2)));
        train_sig1s(:, i+1) = dlmread(strcat('../multi-users-FSK/',save_file,'/train_sig1.txt'));
        long_sig = dlmread(strcat('../multi-users-FSK/',save_file,'/train_sig2.txt'));
        train_sig2s(:, i+1) = long_sig(1:sig_len2);
%         train_idx(:, i+1) = dlmread(strcat('../multi-users-FSK/',save_file,'/FSK_IDX.txt'));
    end
    for i =0:5
        save_file = strcat('N_seq5-', int2str(i), '-', int2str(Ns), '-', int2str(GI), '-', int2str(BW(1)), '-', int2str(BW(2)));
        train_idx(:, i+1) = dlmread(strcat('../multi-users-FSK/',save_file,'/FSK_IDX.txt'));
    end
elseif(sig_type == 3)
    %%% FSK signal
    PN_seq = [1, 1, -1, 1, -1];
    L = length(PN_seq);
    Ns = 1450; %720
    GI = 480; %360
    sig_len1 = Ns;
    sig_len2 = (Ns + GI)*L;

    save_file = strcat('N_seq5_DIFF-', int2str(0), '-', int2str(Ns), '-', int2str(GI), '-', int2str(BW(1)), '-', int2str(BW(2)));
    train_sig1s = zeros(sig_len1, group_size);
    train_sig2s = zeros(sig_len2, group_size);
    train_idx = zeros(Ns, 6);
    file_ids = [1, 5, 11, 22, 35, 49];
    for i =0:group_size-1
        save_file = strcat('N_seq5_DIFF-', int2str(file_ids(i+1)), '-', int2str(Ns), '-', int2str(GI), '-', int2str(BW(1)), '-', int2str(BW(2)));
        train_sig1s(:, i+1) = dlmread(strcat('../multi_user_DIFF/',save_file,'/train_sig1.txt'));
        long_sig = dlmread(strcat('../multi_user_DIFF/',save_file,'/train_sig2.txt'));
        train_sig2s(:, i+1) = long_sig(1:sig_len2);
    end
    for i =0:5
        save_file = strcat('N_seq5_DIFF-', int2str(file_ids(i+1)), '-', int2str(Ns), '-', int2str(GI), '-', int2str(BW(1)), '-', int2str(BW(2)));
        train_idx(:, i+1) = dlmread(strcat('../multi_user_DIFF/',save_file,'/train_pad.txt'));
    end
elseif(sig_type == 2)
    %%% FSK signal
    PN_seq = [1, 1, -1, 1];
    L = length(PN_seq);
    Ns = 1920; %720
    GI = 540; %360
    N_FSK = 1920;
    sig_len1 = Ns;
    sig_len2 = (Ns + GI)*L;
    file_id = '5';
    save_file = strcat('N_seq5-', int2str(0), '-', int2str(Ns), '-', int2str(GI), '-', int2str(BW(1)), '-', int2str(BW(2)));
    train_sig1s = zeros(sig_len1, group_size);
    train_sig2s = zeros(sig_len2, group_size);
    train_idx = zeros(18, 6);

    for i =0:5
        save_file = strcat('N_seq5-', int2str(i), '-', int2str(Ns), '-', int2str(GI), '-', int2str(BW(1)), '-', int2str(BW(2)));
        train_sig1s(:, i+1) = dlmread(strcat('../multi_users_FSK2/',save_file,'/train_sig1.txt'));
        long_sig = dlmread(strcat('../multi_users_FSK2/',save_file,'/train_sig2.txt'));
        train_sig2s(:, i+1) = long_sig(1:sig_len2);
%         train_idx(:, i+1) = dlmread(strcat('../multi-users-FSK/',save_file,'/FSK_IDX.txt'));
    end
    for i =0:5
        save_file = strcat('N_seq5-', int2str(i), '-', int2str(Ns), '-', int2str(GI), '-', int2str(BW(1)), '-', int2str(BW(2)));
        train_idx(:, i+1) = dlmread(strcat('../multi_users_FSK2/',save_file,'/FSK_IDX.txt'));
    end
elseif(sig_type == 6)
    %%% FSK signal
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
        train_sig1s(:, i+1) = dlmread(strcat('../multi_users_FSK3/',save_file,'/train_sig1.txt'));
        long_sig = dlmread(strcat('../multi_users_FSK3/',save_file,'/train_sig2.txt'));
        train_sig2s(:, i+1) = long_sig(1:sig_len2);
%         train_idx(:, i+1) = dlmread(strcat('../multi-users-FSK/',save_file,'/FSK_IDX.txt'));
    end
    for i =0:5
        save_file = strcat('N_seq5-', int2str(i*2), '-', int2str(Ns), '-', int2str(GI), '-', int2str(BW(1)), '-', int2str(BW(2)));
        train_idx(:, i+1) = dlmread(strcat('../multi_users_FSK3/',save_file,'/FSK_IDX.txt'));
    end
end

% 

color_lists = [	"r", "g", 	"b", "c", "m", "y", "k" ];

%% load the recving data to process in reply side
%4.75, 10.77
% roles = ["L0", "U0", "U1", "U2", "U3", "U4"];
% user_ids = [0, 1, 2, 3, 4, 5];


raw_folder = '../../Range_data/harbor/';

for exp_num = 2
    extracted_index = zeros(group_size, 20, group_size);
    valid_meas = ones(20, 1);
    T_min = 9999;
    DIS_gt = zeros(group_size, group_size);
    for r =1:(group_size)
        USER_ID = 4;%group_users(r);
        if USER_ID == 0
            role = 'L0';
        else
            role = strcat('U', int2str(USER_ID-1));
        end
        exp = " ";
        folders = strcat(raw_folder, int2str(exp_num), '/', role, '/');
        if r < 0
            pos_gt = dlmread(strcat(raw_folder, 'gt.txt'));
            
            for i =1:group_size
                for j = i+1:group_size
                    tmp = sqrt(sumsqr(pos_gt(i, :) - pos_gt(j, :)));
                    DIS_gt(i, j) = tmp;
                    DIS_gt(j, i) = tmp;
                end
            end
            DIS_gt
        end
    
        filenames = dir(folders);
        for j = 1:size(filenames, 1)
            name = filenames(j).name;
            if contains(name, "16")
                exp = name;
                break;
            end
        end
        
        rx_signal = dlmread(strcat(folders,  exp, '/', exp, '-',file_id,'-bottom.txt'))/10000;
        rx_signal2 = dlmread(strcat(folders, exp, '/', exp, '-',file_id,'-top.txt'))/10000;
    %     if USER_ID == 0
    %         online_debug = [0, 0, 0];
    %     else
    %         online_debug = dlmread(strcat(folders, exp, '/', exp, '_replyidx.txt'));
    %         online_debug = online_debug(2, :);
    %     end
    %     figure
    %     [acor, lag] = xcorr(rx_signal, train_sig2);
    %     plot(lag, acor)
%         filter_order = 256;
%         wn = [(BW(1) - 100)/(fs/2), (BW(2) + 200)/(fs/2)];    
%         b = fir1(filter_order, wn, 'bandpass');  
%      
%         y_after_fir=filter(b,1,rx_signal);
%         delay_fir = filter_order/2;
%         rx_signal = y_after_fir(delay_fir+1:end);
%         figure
%         plot(rx_signal2)
        if role ~= "L0"
            rx_signal = rx_signal(32*fs+1:end);
            rx_signal2 = rx_signal2(32*fs+1:end); 
        end
    %     figure
    %     hold on
    %     plot(rx_signal)
    %     plot(rx_signal2, '--')
    %     figure
    %     [acor, lag] = xcorr(rx_signal, train_sig2);
    %     plot(lag, acor)
        
        
    %     [naiser_idx, peak, Mn] = naiser_corr3(rx_signal(1877000+1 : 1877000+fs*6), Ns, GI, L, PN_seq);
    %     figure
    %     subplot(211)
    %     plot(rx_signal(1877000+1 : 1877000+fs*6))
    %     subplot(212)
    %     hold on
    %     x = (1:length(Mn))*16;
    %     plot(x, Mn)
    %     ylim([-0.5,1])
        
        [fine_result, coarse_result, rx_signal, rx_signal2] = fine_sync_recv_single(rx_signal,rx_signal2,train_sig1s, train_sig2s, fs, BW, Ns, GI, L, PN_seq, c, USER_ID, group_users, train_idx, save_fig, N_FSK);
        
    %     fine_result(:, :) - fine_result(:, 1)
    %     coarse_result(:, :) - coarse_result(:, 1)
        figure
        hold on
        plot(rx_signal)
        plot(rx_signal2, '--')
        % fine_result + 5*8820
        for i = 1:size(fine_result, 1)
            for j = 1:size(fine_result, 2)
                xline(fine_result(i, j)+270,  '--', "Color",color_lists(j))
            end
        end
        title('reply side synchronization')
        T = size(fine_result, 1);
        for t = 1:T
            if min(fine_result(t, :), [], 'all') < 0
%                 fine_result(t, :)
                valid_meas(t, 1) = 0;
            end
        end
        extracted_index(r, 1:T, :) =  fine_result(:, :) - fine_result(:, 1);
        if(T < T_min)
            T_min = T;
        end
    
    end
    
    T_min
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
    %         dis_matrix - DIS_gt
            dlmwrite(strcat(raw_folder, int2str(exp_num), '/result', int2str(i), '.txt'), dis_matrix );
        end
        
    end

end

