close all
clear 
% clc
fs = 44100;
bias =480;
bias_shift = 240;

BW = [1000, 5000]; %[1000, 6000];
B = BW(2) - BW(1);

PN_seq = [1, 1, -1, 1];
L = length(PN_seq);
Ns = 1920; %720
GI = 540; %360
sig_len1 = Ns;
sig_len2 = (Ns + GI)*L;
file_id = '6';


save_file = strcat('N_seq5-', int2str(0), '-', int2str(Ns), '-', int2str(GI), '-', int2str(BW(1)), '-', int2str(BW(2)));
train_sig1= dlmread(strcat('../multi_users_FSK3/',save_file,'/train_sig1.txt'));
train_sig2 = dlmread(strcat('../multi_users_FSK3/',save_file,'/train_sig2.txt'));
train_sig2 = train_sig2(1:sig_len2);
Ns2 = length(train_sig2);
f_seq = linspace(0, fs-fs/Ns, Ns);


exp_name = 'ofdm10';
% exp = '1685150474482'; % '1688346034162';
% role = 'reply1';
% exp = '1685151648087'; % '1688346034162';
% role = 'reply2';
% exp = '1685150800487'; % '1688346034162';
% role = 'reply3';

exp_name = 'ofdm20';
% exp = '1685151468289'; % '1688346034162';
% role = 'reply1';
% exp = '1685150640539'; % '1688346034162';
% role = 'reply2';
% exp = '1685151846524'; % '1688346034162';
% role = 'reply3';

exp_name = 'ofdm30';
% exp = '1685152699803'; % '1688346034162';
% role = 'reply1';

exp = '1685152867112'; % '1688346034162';
role = 'reply2';


% exp = '1685153022514'; % '1688346034162';
% role = 'reply3';

raw_folder = strcat('../../Range_data/data/',exp_name,'/', role, '/');
rx_signal = dlmread(strcat(raw_folder, '/', exp, '-',file_id,'-bottom.txt'))/10000;
rx_signal = rx_signal(8*fs:end-20000);

filter_order = 256;
wn = [(BW(1) - 100)/(fs/2), (BW(2) + 200)/(fs/2)];    
b = fir1(filter_order, wn, 'bandpass');  



y_after_fir=filter(b,1,rx_signal);
delay_fir = filter_order/2;
rx_signal = y_after_fir(delay_fir+1:end);

figure(122)
hold on
plot(rx_signal)

[acor, lag] = xcorr(rx_signal, train_sig2);
max_number = 40;
%   [acor, lag] = xcorr(rx_signal, train_sig2s(:, 1));
max_peak_corr = max(acor);

begin_idx = 2-lag(1);
acor1 = acor(begin_idx:end);

[max_peak_corr, max_id] = max(acor1);

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
variance_sample = 1500;
look_back = 1000;
sig_len = Ns2 + variance_sample*2;

idx = chunk_size;
result_index = [];
figure(12)
hold on
plot(acor1)
scatter(gt_ids, acor1(gt_ids), 'bo')

while 1
    if idx + chunk_size*2 + 1> length(acor1) || (idx + chunk_size*2 + sig_len)> length(acor1)
        break
    end
    seg = acor1(idx + 1 : idx + chunk_size*2);
    [max_val, max_id] = max(seg);
    if (max_val > threshold)
        self_idx = idx + max_id;
        sig = rx_signal(self_idx + 1-variance_sample: self_idx + sig_len);
        [naiser_idx, peak, Mn] = naiser_corr3(sig, Ns, GI, L, PN_seq);
%         if self_idx > 1300000
%             figure(11)
%             plot(Mn)
%             figure(10)
%             plot(sig)
%            
%             self_idx
%         end
        if peak > 0.2 && naiser_idx > 0
            result_index = [result_index,  naiser_idx + self_idx - variance_sample];
            figure(12)
            scatter(result_index(end), acor1(result_index(end)), 'rx')
            idx = idx + 4*chunk_size;
        else
            idx = idx + chunk_size;
        end
    else
        idx = idx + chunk_size;
        
    end
end




false_postitive = 0;
true_positive = 0;
err_th = 2000;

for i = 1:length(result_index)
    current_id = result_index(i);
    err = min(abs(gt_ids - current_id));
    if err <= err_th
        true_positive = true_positive +1;
    else
        false_postitive = false_postitive +1;
    end
end

false_negative = length(gt_ids) - true_positive;
false_postitive
false_negative
true_positive

true_positive/(false_postitive + true_positive)
true_positive/(false_negative + true_positive)

