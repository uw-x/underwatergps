function user_id = check_FSK(sig,train_idxs)
%UNTITLED2 Summary of this function goes here
%   Detailed explanation goes here
spect = abs(fft(sig));
% figure
% subplot(211)
% plot(spect)
% subplot(212)
% plot(sig)

avg_snr = [];
if(size(train_idxs, 1) == 16)
    nearby = 12;
else
    nearby = 7;
end
for i=1:size(train_idxs, 2)
    idx = train_idxs(:, i);

    SNRS = [];
    for j = 1:size(idx, 1)
        ix = idx(j);
        snr = spect(ix, 1)*2/(mean(spect(ix-nearby:ix-1,1)) + mean(spect(ix+1:ix+nearby,1)));
        SNRS = [SNRS, mag2db(snr)];
    end
    
    avg_snr= [avg_snr mean(SNRS)];
end
% avg_snr
[v, user_id] = max(avg_snr);

end