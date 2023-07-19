close all
clear 
% clc
fs = 44100;

BW = [1000, 5000]; %[1000, 6000];
B = BW(2) - BW(1);

save_file = strcat('../FMCW/');
train_sig = dlmread(strcat(save_file,'/train_sig.txt'));
Ns = length(train_sig);
f_seq = linspace(0, fs-fs/Ns, Ns);
file_id = '7';


exps = [];
nums = [];
roles = [];

exp_name = 'fmcw10';
exp = '1685150976164'; % '1688346034162';
role = 'reply1';
exps = [exps; exp_name];
nums = [nums; exp];
roles = [roles; role];

exp = '1685151130077'; % '1688346034162';
role = 'reply2';
exps = [exps; exp_name];
nums = [nums; exp];
roles = [roles; role];

exp = '1685151289428'; % '1688346034162';
role = 'reply3';
exps = [exps; exp_name];
nums = [nums; exp];
roles = [roles; role];

exp_name = 'fmcw20';
exp = '1685152164591'; % '1688346034162';
role = 'reply1';
exps = [exps; exp_name];
nums = [nums; exp];
roles = [roles; role];

exp = '1685152327473'; % '1688346034162';
role = 'reply2';
exps = [exps; exp_name];
nums = [nums; exp];
roles = [roles; role];

exp = '1685152489434'; % '1688346034162';
role = 'reply3';
exps = [exps; exp_name];
nums = [nums; exp];
roles = [roles; role];

exp_name = 'fmcw30';
exp = '1685153189478'; % '1688346034162';
role = 'reply1';
exps = [exps; exp_name];
nums = [nums; exp];
roles = [roles; role];

exp = '1685153347370'; % '1688346034162';
role = 'reply2';
exps = [exps; exp_name];
nums = [nums; exp];
roles = [roles; role];

exp = '1685153504068'; % '1688346034162';
role = 'reply3';
exps = [exps; exp_name];
nums = [nums; exp];
roles = [roles; role];
TH  = 1000;

false_postitive = 0;
true_positive = 0;
false_negative = 0;

for num_i = 1:length(exps)
    
    exp_name = exps(num_i, :);
    exp = nums(num_i, :);
    role = roles(num_i, :);
    
    raw_folder = strcat('../../Range_data/data/',exp_name,'/', role, '/');
    rx_signal = dlmread(strcat(raw_folder, '/', exp, '-',file_id,'-bottom.txt'))/10000;
    rx_signal = rx_signal(8*fs:end-20000);
    
    filter_order = 256;
    wn = [(BW(1) - 100)/(fs/2), (BW(2) + 200)/(fs/2)];    
    b = fir1(filter_order, wn, 'bandpass');  
    
    y_after_fir=filter(b,1,rx_signal);
    delay_fir = filter_order/2;
    rx_signal = y_after_fir(delay_fir+1:end);
    
%     figure(122)
%     
%     hold on
%     plot(rx_signal)
    
    [acor, lag] = xcorr(rx_signal, train_sig);
    max_number = 40;
    %   [acor, lag] = xcorr(rx_signal, train_sig2s(:, 1));
    [pks,locs]=findpeaks(acor,'MinPeakHeight',5,'MinPeakDistance',44100);
    
    begin_idx = 2-lag(1);
    acor1 = acor(begin_idx:end);
    locs0 = lag(locs);
    
    negative_index = locs0<=0;
    locs0(negative_index) =[];
    
    
    % max_id = 3116310; % 10m - r1
    % max_id = 734000; % 10m - r3
    % max_id = 1709430; % 20m
    % max_id = 1345700; % 30m
    max_id = dlmread(strcat(raw_folder, '/gt_id.txt'));
    gt_interval = 2*fs;
    gt0 = mod(max_id, gt_interval);
    gt_id = gt0;
    gt_ids = [gt0];
    while(gt_id(end) + gt_interval < length(acor1))
        gt_id = gt_id + gt_interval;
        gt_ids = [gt_ids, gt_id];
    end
    
    chunk_size = 0.25*fs;
    threshold = 0.2;
    
    idx = chunk_size;
    result_index = [];
    
    
    while 1
        if idx + chunk_size*2 + 1> length(acor1)
            break
        end
        seg = acor1(idx + 1 : idx + chunk_size*2);
    %     figure(11)
    %     plot(seg)
        [max_val, max_id] = max(seg);
        power_now = sumsqr(acor1(idx + max_id - 50: idx + max_id + 50))/100;
        power_pre = sumsqr(acor1(idx + max_id - Ns: idx + max_id - Ns + 100))/100;
    
    
        if (max_val > threshold) && (power_now/power_pre) > TH
            result_index = [result_index,  max_id + idx];
            
            idx = idx + chunk_size*4;
        else
            idx = idx + chunk_size;
        end    
    end
    
    figure()
    hold on
    plot(acor1)
    scatter(gt_ids, acor1(gt_ids), 'bo')
    scatter(result_index, acor1(result_index), 'rx')
    
    

    err_th = 2000;
    true_positive0 = 0;
    for i = 1:length(result_index)
        current_id = result_index(i);
        err = min(abs(gt_ids - current_id));
        if err <= err_th
            true_positive = true_positive +1;
            true_positive0 = true_positive0 + 1;
        else
            false_postitive = false_postitive +1;
        end
    end
    false_negative = false_negative + length(gt_ids) - true_positive0;
end

false_postitive
false_negative
true_positive

true_positive/(false_postitive + true_positive)
true_positive/(false_negative + true_positive)